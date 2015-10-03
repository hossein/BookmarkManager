#include "MHTSaver.h"

#include "Util.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QMimeDatabase>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QTimer>

MHTSaver::MHTSaver(QObject *parent) :
    QObject(parent)
{
    qnam = new QNetworkAccessManager(this);

    m_useOverallTimer = false;
    m_overallTimer = new QTimer(this);
    connect(m_overallTimer, SIGNAL(timeout()), this, SLOT(OverallTimerTimeout()));

    m_stripJS = true;

    //Content types
    m_htmlContentTypes       = QStringList() << "text/html";
    m_scriptContentTypes     = QStringList() << "application/ecmascript" << "application/javascript" << "application/x-javascript" << "text/javascript";
    m_cssContentTypes        = QStringList() << "text/css";
    m_otherKnownContentTypes = QStringList();
            /// /* Images    */ << "image/gif" << "image/jpeg" << "image/pjpeg" << "image/png" << "image/x-icon" << "image/vnd.microsoft.icon" << "image/svg+xml"
            /// /* Text      */ << "text/plain"
            /// /* XML, etc  */ << "text/xml" << "application/atom+xml" << "application/rss+xml" << "application/rdf+xml" << "application/opensearchdescription+xml"
            /// /* Documents */ << "application/pdf"
            /// /* Fonts     */ << "font/woff" << "application/x-font-woff" << "application/x-font-ttf" << "application/vnd.ms-fontobject";
}

MHTSaver::~MHTSaver()
{

}

int MHTSaver::overallTimeoutTime() const
{
    return m_useOverallTimer ? m_overallTimer->interval() / 1000 : -1;
}

bool MHTSaver::stripJS() const
{
    return m_stripJS;
}

void MHTSaver::setOverallTimeoutTime(int seconds)
{
    m_useOverallTimer = (seconds > 0);
    if (seconds > 0)
        m_overallTimer->setInterval(seconds * 1000);
}

void MHTSaver::setStripJS(bool strip)
{
    m_stripJS = strip;
}

void MHTSaver::OverallTimerTimeout()
{
    Cancel();
}

void MHTSaver::GetMHTData(const QString& url)
{
    m_resources.clear();
    m_ongoingReplies.clear();
    m_status.mainSuccess = false;
    m_status.mainHttpErrorCode = -1;
    m_status.mainNetworkReplyError = QNetworkReply::NoError;
    m_status.mainNetworkReplyErrorString = QString();
    m_status.mainResourceTitle = QString();
    m_status.fileSuffix = QString();
    m_status.resourceCount = 0;
    m_status.resourceSuccess = 0;
    m_cancel = false;

    //Start our timer
    if (m_useOverallTimer)
        m_overallTimer->start();

    //Load each resource with `LoadResource`; after response comes they will be added to resources
    //  with `AddResource`.
    LoadResource(QUrl(url));
}

void MHTSaver::Cancel()
{
    m_cancel = true;
    foreach (QNetworkReply* reply, m_ongoingReplies.keys())
        reply->abort(); //`finished()` will also be emitted, so don't remove them.
}

void MHTSaver::LoadResource(const QUrl& url)
{
    //Note: We do differentiate the same url with different fragments (i.e the `#blahblah` part of the url)
    //      as it is unlikely they happen in links to external resources, and if they happen they may be
    //      used by different libraries for content differentiation, or e.g hashtags.
    ///if (!url.fragment().isEmpty())
    ///    qDebug() << "LOAD: QUrl has fragment: " << url;

    //Check if resource is already loaded or is scheduled for loading.
    foreach (const Resource& res, m_resources)
        if (res.fullUrl == url)
            return;
    foreach (const QNetworkReply* reply, m_ongoingReplies.keys())
        if (reply->url() == url)
            return;

    QNetworkRequest req;
    req.setUrl(url);
    QString chromeUserAgent = "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/33.0.1750.154 Safari/537.36";
    req.setHeader(QNetworkRequest::UserAgentHeader, chromeUserAgent);

    QNetworkReply* reply = qnam->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(ResourceLoadingFinished()));

    //Always set progress THEN the timestamp, to make sure if progress changes in between,
    //  next call will update the timestamp. Actually I don't think it differs so much.
    OngoingReply ongoingReply;
    ongoingReply.progress = -1;
    ongoingReply.progressTimeStamp = QDateTime::currentDateTime();
    m_ongoingReplies.insert(reply, ongoingReply);
}

void MHTSaver::ResourceLoadingFinished()
{
    QNetworkReply* reply = dynamic_cast<QNetworkReply*>(sender());

    if (m_cancel) //No need to check if reply is valid for this.
    {
        //Stop early
        DeleteReplyAndCheckForFinish(reply);
        return;
    }

    //The first resource's error is important.
    bool isMainResource = m_resources.isEmpty();
    if (isMainResource)
    {
        m_status.mainSuccess = (reply->error() == QNetworkReply::NoError);
        //Since we don't add the resource, in case of redirects, this will be overwritten.
        m_status.mainHttpErrorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_status.mainNetworkReplyError = reply->error();
        m_status.mainNetworkReplyErrorString = reply->errorString();

        if (reply->error() != QNetworkReply::NoError)
        {
            //Don't continue. We're done. Just emit an error.
            return DeleteReplyAndCheckForFinish(reply); //Handles this error case also.
        }
    }

    if (reply->error() != QNetworkReply::NoError)
        return DeleteReplyAndCheckForFinish(reply);

    QString redirectLocation = QString();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus == 301 || httpStatus == 302 || httpStatus == 303 || httpStatus == 307 || httpStatus == 308)
    {
        //Redirect!
        //HOWEVER, if this is not the main page save all interim things in the archive too;
        //  although e.g for pictures they won't be displayed as we are NOT modifying the image
        //  addresses or save them under the old address.
        //Only for the main page we fully replace the redirect target with the original thing user
        //  passed in; this also changes main http status code and main error, for other resources
        //  the function continues and the `AddResource` call adds the resource to our list.
        redirectLocation = reply->header(QNetworkRequest::LocationHeader).toString();
        ///qDebug() << "LOAD: Redirect to: " << redirectLocation;
        LoadResource(QUrl(redirectLocation));
        if (isMainResource)
            return DeleteReplyAndCheckForFinish(reply);
    }

    if (httpStatus < 200 || httpStatus >= 400)
    {
        //For debugging purposes only. An invalid URL can be a sign of bad parsing from our side.
        qDebug() << "LOAD: HTTP ERROR " << httpStatus << " for resource: " << reply->url();
    }

    //Add the resource.
    QString contentType = AddResource(reply, redirectLocation);

    //Determine the file type and add any more contents.
    contentType = getRawContentType(contentType);
    ///qDebug() << "LOAD: ContentType is " << contentType;

    QUrl url = reply->url();
    if (!redirectLocation.isEmpty())
    {
        //If a redirect, don't parse it at all. Only exit the if/else ladder and go to end of the function.
    }
    else if (m_htmlContentTypes.contains(contentType))
    {
        //A common error is to have css or js files with this content type. check extesnsion here.
        //  Actually any thing including images and downloaded files can come here too but we don't
        //  check them.
        //We use url's `path()` to make sure it doesn't contain query string or fragments.
        if (url.path().right(4).toLower() == ".css")
            ParseAndAddCSSResources(reply);
        else if (url.path().right(3).toLower() == ".js")
            {} //Scripts don't need parsing.
        else
            ParseAndAddHTMLResources(reply); //Now normal html
    }
    else if (m_cssContentTypes.contains(contentType))
    {
        ParseAndAddCSSResources(reply);
    }
    else if (m_scriptContentTypes.contains(contentType)
          || m_otherKnownContentTypes.contains(contentType))
    {
        //Other known resource types. Has been added to resources list. No special processing.
    }
    else //includes `if (contentType.isNull())`
    {
        //Unknown resources. Has been added to resources list. We check for some additional extensions.

        ///if (contentType.isNull())
        ///    qDebug() << "LOAD: " << url << " doesn't have content type specified.";
        ///else
        ///    qDebug() << "LOAD: Unknown content type '" << contentType << "' for resource: "<< url;

        //A common error is to have css files produced e.g with php but not sending the correct
        //  content-type header.
        //  Actually any thing including images and downloaded files can come here too but we don't
        //  check them.
        //We use url's `path()` to make sure it doesn't contain query string or fragments.
        if (url.path().right(4).toLower() == ".css")
            ParseAndAddCSSResources(reply);
    }

    //For the main page, the `ParseAndAddHTMLResources` should set its title if it's an html page.
    //However if it doesn't have a title tag, or if it's not html (i.e maybe it's an image), we set
    //  some title to not let it be empty. We use QFileInfo and it may be able to extract some name
    //  for the file.
    if (isMainResource && m_status.mainResourceTitle.isEmpty())
    {
        //We use `completeBaseName()` not `fileName`, this will create 'index.mhtml' instead
        //  of index.html.mhtml, and also 'archive.tar.gz' instead of 'archive.tar.gz.gz'.
        m_status.mainResourceTitle = QFileInfo(url.path()).completeBaseName();
        if (m_status.mainResourceTitle.isEmpty()) //Still empty?
            m_status.mainResourceTitle = "File";
    }

    DeleteReplyAndCheckForFinish(reply);
}

void MHTSaver::DeleteReplyAndCheckForFinish(QNetworkReply* reply)
{
    //IMPORTANT: Don't forget!
    reply->deleteLater();
    m_ongoingReplies.remove(reply); //Remove from list of ongoing.

    if (m_cancel)
    {
        //If user cancelled, emit the special status when resources finish.
        if (m_ongoingReplies.size() == 0)
        {
            m_status.mainSuccess = false;
            m_status.mainHttpErrorCode = 0;
            m_overallTimer->stop();
            emit MHTDataReady(QByteArray(), m_status);
        }
    }
    else if (m_resources.isEmpty() && reply->error() != QNetworkReply::NoError)
    {
        //If this is an error on loading the first, i.e the main resource
        m_overallTimer->stop();
        emit MHTDataReady(QByteArray(), m_status);
    }
    else if (m_ongoingReplies.size() == 0)
    {
        //Otherwise if things finished, generate the MHT.
        m_overallTimer->stop();
        GenerateMHT();
    }
}

QString MHTSaver::AddResource(QNetworkReply* reply, const QString& redirectLocation)
{
    QUrl url = reply->url();

    foreach (const Resource& res, m_resources)
    {
        if (res.fullUrl == url)
        {
            qDebug() << "ASSERTION FAILED! Resource " << url << " already exists!";
            return QString();
        }
    }

    const QByteArray data = reply->peek(reply->bytesAvailable());
    QString contentType = getOrGuessMimeType(reply, data);
    //We don't strip the 'charset=' part of the content type here.

    Resource res;
    res.fullUrl = url;
    res.contentType = contentType;
    res.data = data;
    if (!redirectLocation.isEmpty())
        res.redirectTo = QUrl(redirectLocation);

    m_resources.append(res);
    m_status.resourceCount += 1;
    m_status.resourceSuccess += (reply->error() == QNetworkReply::NoError ? 1 : 0);

    return contentType;
}

void MHTSaver::ParseAndAddHTMLResources(QNetworkReply* reply)
{
    QUrl url = reply->url();
    QByteArray data = reply->readAll();
    QString str = QString::fromUtf8(data);

    //Regular expressions stolen from the following and modified a bit:
    //http://www.codeproject.com/Articles/8268/Convert-any-URL-to-a-MHTML-archive-using-native-NE

    //This will also catch the (i)frames recursively, as we automatically load all htmls' resources.
    QString pattern = "(\\s(src|background)\\s*=\\s*|<link[^>]+?href\\s*=\\s*)"
                      "(?|'(?<url>[^'\\n\\r]+)|\"(?<url>[^\"\\n\\r]+)|(?<url>[^ \\n\\r]+))";
    QRegularExpression regexp(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = regexp.globalMatch(str);

    ///qDebug() << "PARSE HTML: Base: " << url;
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(url, linkedUrl);
    }

    //Add the resources referenced in <style> tags css too. The most notable use is for google
    //  search results. (We do nothing about 'style=' attributes though.)
    QByteArray htmlStyles = getHTMLStyles(data);
    if (!htmlStyles.isEmpty())
        ParseAndAddInlineCSSResources(url, htmlStyles);

    //Also, if this is the first resource, get the html <title> from it.
    if (m_resources.size() == 1)
    {
        QString titlePattern = "<title>(?<title>[^<]*)";
        QRegularExpression titleRegexp(titlePattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = titleRegexp.match(str);
        if (match.isValid())
            m_status.mainResourceTitle = match.captured("title").toUtf8();
    }
}

void MHTSaver::ParseAndAddCSSResources(QNetworkReply* reply)
{
    QUrl url = reply->url();
    QByteArray data = reply->readAll();
    ParseAndAddInlineCSSResources(url, data);
}

void MHTSaver::ParseAndAddInlineCSSResources(const QUrl& baseUrl, const QByteArray& style)
{
    QString str = QString::fromUtf8(style);

    //The following QUESTION MARK actually makes our pattern recognize additional 'url('s that are
    //  not part of an import/*-image/background.
    //This QUESTION MARK HERE ---------------------------v
    QString pattern = "(@import\\s|\\S+-image:|background:)\\s*?(url)?\\s*([\"'(]{1,2}(?<url>[^\"')]+)[\"')]{1,2})";
    QRegularExpression regexp(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = regexp.globalMatch(str);

    QString pattern2 = "url\\(\\s*[\"']?(?<url>[^\"'\\)]+)";
    QRegularExpression regexp2(pattern2, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches2 = regexp2.globalMatch(str);

    //Paths for CSS will be fine:
    //http://stackoverflow.com/questions/940451/using-relative-url-in-css-file-what-location-is-it-relative-to

    ///qDebug() << "PARSE CSS: Base: " << baseUrl;
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(baseUrl, linkedUrl);
    }
    while (matches2.hasNext())
    {
        QRegularExpressionMatch match = matches2.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(baseUrl, linkedUrl);
    }
}

void MHTSaver::DecideAndLoadURL(const QUrl& baseURL, const QString& linkedURL)
{
    QString finalURL;
    if (linkedURL.contains("://"))
    {
        //External url
        finalURL = linkedURL;
    }
    else if (linkedURL[0] == '/')
    {
        //Absolute URL
        finalURL = baseURL.scheme() + "://" + baseURL.authority();
        finalURL += linkedURL; //linkedURL already has '/' in its beginning.
    }
    else
    {
        //Relative URL
        QString basePath = baseURL.path();

        //Do NOT ignore trailing slashes. It doesn't differ for 'cp.ir/index.php' and 'cp.ir/',
        //  BUT IT DOES DIFFER FOR 'cp.ir/fanegar/' and 'cp.ir/fanegar/index.php'.
        //  Use basePath as  it is.
        //First remove the last slashed part (even if it's a last character):
        int indexOfLastSlash = basePath.lastIndexOf('/');
        basePath = (indexOfLastSlash == -1 ? QString() : basePath.left(indexOfLastSlash));

        //Then while we have "../" in the linkedURL remove more parts:
        QString finalLinkedURL = linkedURL;
        while (finalLinkedURL.length() >= 3 && finalLinkedURL.left(3) == "../")
        {
            indexOfLastSlash = basePath.lastIndexOf('/');
            if (indexOfLastSlash != -1)
            {
                //Again remove a part from basePath, and the '../' from the finalLinkedURL
                basePath = basePath.left(indexOfLastSlash);
                finalLinkedURL = finalLinkedURL.mid(3);
            }
            else
            {
                //Wrong URL here, don't touch anything now.
                break;
            }
        }

        finalURL = baseURL.scheme() + "://" + baseURL.authority()
                 + basePath + "/" + finalLinkedURL;
    }

    ///qDebug() << "DECIDE: " << linkedURL << " => " << finalURL;
    LoadResource(QUrl(finalURL));
}

void MHTSaver::GenerateMHT()
{
    //Don't generate an mhtml file for single files.
    if (m_resources.size() == 1 && !m_htmlContentTypes.contains(getRawContentType(m_resources[0].contentType)))
    {
        GenerateFile();
        return;
    }

    const QByteArray boundary = "----=_NextPart_000_00";

    QByteArray mhtdata;
    mhtdata += "From: <Saved by BookmarkManager>\r\n";

    Util::QEncodingOptions qenc;
    qenc.QEncoding = true;
    qenc.firstLineAdditionalLen = 9;
    qenc.originalEncoding = "utf-8";
    //Now this is one place where we don't support any encoding other than UTF-8.
    QByteArray subjectQEncoded = Util::EncodeQuotedPrintable(m_status.mainResourceTitle.toUtf8(), false, qenc);
    mhtdata += "Subject: " + subjectQEncoded + "\r\n";

    QLocale usLocale(QLocale::English, QLocale::UnitedStates);
    QString currDateStr = usLocale.toString(QDateTime::currentDateTime(), ("ddd, dd MMM yyyy HH:mm:ss t"));
    mhtdata += "Date: " + currDateStr.toLatin1() + "\r\n";

    mhtdata += "MIME-Version: 1.0\r\n";

    QString firstResourceContentType = getRawContentType(m_resources[0].contentType);
    mhtdata += "Content-Type: multipart/related;\r\n"
               "\ttype=\"" + firstResourceContentType.toLatin1() + "\";\r\n"
               "\tboundary=\"" + boundary + "\"\r\n";

    mhtdata += "X-Producer: BookmarkManager\r\n\r\n";

    mhtdata += "This is a multi-part message in MIME format.\r\n\r\n";

    foreach (const Resource& res, m_resources)
    {
        QUrl currentRedirectUrl;
        QString contentType;
        QByteArray data;

        contentType = res.contentType;
        data = res.data;

        //First off, if the file was a redirect replace original data and content type with the
        //  redirect target. We don't replace the url as that requires changing files' source.
        //  We don't remove the target resource or something. It will mess up m_resources list.
        QUrl redirectTo = res.redirectTo;
        while (!redirectTo.isEmpty())
        {
            int i = findResourceWithURL(redirectTo);
            if (i == -1)
                break;

            currentRedirectUrl = m_resources[i].fullUrl;
            contentType = m_resources[i].contentType;
            data = m_resources[i].data;

            //Check for more redirect depths.
            redirectTo = m_resources[i].redirectTo;
        }

        //Write the headers
        QString rawContentType = getRawContentType(contentType);
        bool isText = isMimeTypeTextFile(rawContentType);

        mhtdata += "--" + boundary + "\r\n";

        if (!contentType.isEmpty())
            mhtdata += "Content-Type: " + contentType.toLatin1() + "\r\n";

        mhtdata += "Content-Transfer-Encoding: ";
        mhtdata += (isText ? "quoted-printable" : "base64");
        mhtdata += "\r\n";

        mhtdata += "Content-Location: " + res.fullUrl.toString(QUrl::FullyEncoded) + "\r\n";

        if (!currentRedirectUrl.isEmpty())
        {
            mhtdata += "X-BM-IsRedirect: Yes\r\n";
            mhtdata += "X-BM-RedirectTarget: " + currentRedirectUrl.toString(QUrl::FullyEncoded)  + "\r\n";
            mhtdata += "X-BM-OriginalContentType: " + res.contentType.toLatin1()  + "\r\n";
        }

        mhtdata += "\r\n";

        //Strip JS
        if (m_stripJS && m_htmlContentTypes.contains(rawContentType))
        {
            data = stripHTMLScripts(data);
        }
        else if (m_stripJS && m_scriptContentTypes.contains(rawContentType))
        {
            data = QByteArray("/* Script removed by snapshot save */\n");
        }

        //Write content
        if (isText)
        {
            mhtdata += Util::EncodeQuotedPrintable(data);
        }
        else
        {
            QByteArray base64 = data.toBase64();
            QByteArray base64lines;
            for (int i = 0; i <= base64.length() / 76; i++)
                base64lines += base64.mid(i * 76, ((i+1)*76 > base64.length() ? -1 : 76)) + "\r\n";
            mhtdata += base64lines;
        }
        mhtdata += "\r\n";
    }

    //Last one has an additional '--' at end.
    mhtdata += "--" + boundary + "--\r\n";

    m_status.fileSuffix = "mhtml";
    emit MHTDataReady(mhtdata, m_status);
}

void MHTSaver::GenerateFile()
{
    //We have used `completeBaseName()` before. Now we use only 'suffix()'.
    QString suffix = QFileInfo(m_resources[0].fullUrl.path()).suffix();

    //If doesn't have a suffix, try to find out its extension based on mime type.
    if (suffix.isEmpty())
    {
        const QMimeType mimeType = QMimeDatabase().mimeTypeForFileNameAndData(
                    QFileInfo(m_resources[0].fullUrl.path()).fileName(), m_resources[0].data);
        suffix = mimeType.preferredSuffix();
    }

    m_status.fileSuffix = suffix; //May be empty
    emit MHTDataReady(m_resources[0].data, m_status);
}

int MHTSaver::findResourceWithURL(const QUrl& url)
{
    for (int i = 0; i < m_resources.size(); i++)
        if (m_resources[i].fullUrl == url)
            return i;
    return -1;
}

QString MHTSaver::getRawContentType(const QString& contentType)
{
    //ContentType is usually 'text/html; charset=UTF-8' for web pages (gzipped and chunked contents
    //  are already handled and removed)
    int indexOfSemicolon = contentType.indexOf(';');
    if (indexOfSemicolon != -1)
        return contentType.left(indexOfSemicolon);
    else
        return contentType;
}

bool MHTSaver::isMimeTypeTextFile(const QString& mimeType)
{
    QStringList applicationTextTypes = QStringList()
        << "application/atom+xml"
        << "application/ecmascript"
        << "application/json"
        << "application/javascript"
        << "application/x-javascript"
        << "application/rss+xml"
        << "application/xhtml+xml"
        << "application/xml";

    if (applicationTextTypes.contains(mimeType.toLower()))
    {
        return true;
    }
    else if (mimeType.length() >= 5 && mimeType.left(5).toLower() == "text/")
    {
        return true;
    }

    return false;
}

QString MHTSaver::getOrGuessMimeType(QNetworkReply* reply, const QByteArray& data)
{
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (contentType.isEmpty())
    {
        const QMimeType mimeType = QMimeDatabase().mimeTypeForFileNameAndData(
                    QFileInfo(reply->url().path()).fileName(), data);
        contentType = mimeType.name();
    }
    return contentType;
}

QByteArray MHTSaver::getHTMLStyles(const QByteArray& html)
{
    //Limitations:
    //  Read `MHTSaver::stripHTMLScripts`'s limitations.

    QByteArray output;
    int startIndex = 0;

    //In each round find a style tag and copy data inside it.
    while (true)
    {
        //Find '<style'
        int openStyleTagIndex = html.indexOf(QByteArray("<style"), startIndex);
        if (openStyleTagIndex == -1)
        {
            break;
        }

        //Find '>'
        int openStyleTagUntil = html.indexOf('>', openStyleTagIndex);
        if (openStyleTagUntil == -1)
        {
            break;
        }

        int styleContentBeginning = openStyleTagUntil + 1; // `+ 1` to consume the '>' itself

        //Find '</style>'
        int closeStyleTagIndex = html.indexOf(QByteArray("</style>"), styleContentBeginning);
        if (closeStyleTagIndex == -1)
        {
            //Unclosed style. Maybe tempted to copy it! Yes do it! Syntax errors are not a big deal here.
            output += html.mid(styleContentBeginning);
            break;
        }
        else
        {
            output += html.mid(styleContentBeginning, closeStyleTagIndex - styleContentBeginning);
            output += " "; //Try to separate them.
        }

        //Skip the closing style tag to get ready for next round
        startIndex = closeStyleTagIndex + 8; //8 is len("</style>")
    }

    return output;
}

QByteArray MHTSaver::stripHTMLScripts(const QByteArray& html)
{
    //Limitations:
    //- Doesn't support encodings
    //- Doesn't strip non-lowercase scripts
    //- This is primitive, doesn't parse the html file. E.g may get caught by '<script> being
    //  inside a string.

    QByteArray output;
    int startIndex = 0;

    //In each round, copy data before script, the script tag, then the 'removed' message and the
    //  closing script tag. If in a round a script tag was not found, copy til the end and return.
    while (true)
    {
        //Find '<script'
        int openScriptTagIndex = html.indexOf(QByteArray("<script"), startIndex);
        if (openScriptTagIndex == -1)
        {
            output += html.mid(startIndex);
            break;
        }

        //Find '>'
        int openScriptTagUntil = html.indexOf('>', openScriptTagIndex);
        if (openScriptTagUntil == -1)
        {
            output += html.mid(startIndex);
            break;
        }

        int scriptContentBeginning = openScriptTagUntil + 1; // `+ 1` to consume the '>' itself

        //Copy until the script content beginning
        output += html.mid(startIndex, scriptContentBeginning - startIndex);
        //Add the 'removed' message.
        //Unlike MAFF, we do not add '\n's here. Maybe this is in a JS string!
        output += QByteArray("<!-- /* Script removed by snapshot save */ -->");

        //Find '</script>'
        int closeScriptTagIndex = html.indexOf(QByteArray("</script>"), scriptContentBeginning);
        if (closeScriptTagIndex == -1)
        {
            //Unclosed script. Maybe tempted to not copy anything, but we'd better copy everything
            //  til the end to avoid data loss.
            output += html.mid(scriptContentBeginning);
            break;
        }

        //"Copy" the closing script tag
        output += QByteArray("</script>");

        //Skip it to get ready for next round
        startIndex = closeScriptTagIndex + 9; //9 is len("</script>")
    }

    return output;
}
