#include "MHTSaver.h"

#include "Util.h"
#include <QRegularExpression>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

MHTSaver::MHTSaver(QObject *parent) :
    QObject(parent)
{
    qnam = new QNetworkAccessManager(this);

    //Content types
    m_htmlContentTypes       = QStringList() << "text/html";
    m_scriptContentTypes     = QStringList() << "application/ecmascript" << "application/javascript" << "text/javascript";
    m_cssContentTypes        = QStringList() << "text/css";
    m_otherKnownContentTypes = QStringList() << "image/gif" << "image/jpeg" << "image/png";
}

MHTSaver::~MHTSaver()
{

}

void MHTSaver::GetMHTData(const QString& url)
{
    m_resources.clear();
    m_ongoingReplies.clear();
    m_status.mainSuccess = false;
    m_status.mainHttpErrorCode = -1;
    m_status.mainResourceTitle = QByteArray();
    m_status.mainNetworkReplyError = QNetworkReply::NoError;
    m_status.mainNetworkReplyErrorString = QString();
    m_status.resourceCount = 0;
    m_status.resourceSuccess = 0;
    m_cancel = false;

    //Load each resource with `LoadResource`; after response comes they will be added to resources
    //  with `AddResource`.
    LoadResource(QUrl(url));
}

void MHTSaver::Cancel()
{
    m_cancel = true;
}

void MHTSaver::LoadResource(const QUrl& url)
{
    //Note: We do differentiate the same url with different fragments (i.e the `#blahblah` part of the url)
    //      as it is unlikely they happen in links to external resources.
    if (!url.fragment().isEmpty())
        qDebug() << "LOAD: QUrl has fragment: " << url;

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

    if (m_cancel)
    {
        //Stop early
        DeleteReplyAndCheckForFinish(reply);
        return;
    }

    //The first resource's error is important.
    if (m_resources.isEmpty())
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

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus == 301 || httpStatus == 302 || httpStatus == 303 || httpStatus == 307 || httpStatus == 308)
    {
        //Redirect!
        //HOWEVER, if this is not the main page save all interim things in the archive too;
        //  although e.g for pictures they won't be displayed as we are NOT modifying the image
        //  addresses or save them under the old address.
        //Only for the main page we fully replace the redirect target with the original thing user passed in.
        QString newLocation = reply->header(QNetworkRequest::LocationHeader).toString();
        qDebug() << "LOAD: Redirect to: " << newLocation;
        LoadResource(QUrl(newLocation));
        if (m_resources.isEmpty())
            return DeleteReplyAndCheckForFinish(reply);
    }

    //Add the resource.
    AddResource(reply);

    //Determine the file type and add any more contents.
    QUrl url = reply->url();
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << "LOAD: ContentType is " << contentType;

    if (m_htmlContentTypes.contains(contentType))
    {
        //A common error is to have css or js files with this content type. check extesnsion here.
        //  Actually any thing including images and downloaded files can come here too but we don't
        //  check them.
        //We use url's `path()` to make sure it doesn't contain query string or fragments.
        if (reply->url().path().right(4).toLower() == ".css")
            ParseAndAddCSSResources(reply);
        else if (reply->url().path().right(3).toLower() == ".js")
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

        if (contentType.isNull())
            qDebug() << "LOAD: " << url << " doesn't have content type specified.";
        else
            qDebug() << "LOAD: Unknown content type '" << contentType << "' for resource: "<< url;

        //A common error is to have css files produced e.g with php but not sending the correct
        //  content-type header.
        //  Actually any thing including images and downloaded files can come here too but we don't
        //  check them.
        //We use url's `path()` to make sure it doesn't contain query string or fragments.
        if (reply->url().path().right(4).toLower() == ".css")
            ParseAndAddCSSResources(reply);
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
            emit MHTDataReady(QByteArray(), m_status);
        }
    }
    else if (m_resources.isEmpty() && reply->error() != QNetworkReply::NoError)
    {
        //If this is an error on loading the first, i.e the main resource
        emit MHTDataReady(QByteArray(), m_status);
    }
    else if (m_ongoingReplies.size() == 0)
    {
        //Otherwise if things finished, generate the MHT.
        GenerateMHT();
    }
}

void MHTSaver::AddResource(QNetworkReply* reply)
{
    QUrl url = reply->url();
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    foreach (const Resource& res, m_resources)
    {
        if (res.fullUrl == url)
        {
            qDebug() << "ASSERTION FAILED! Resource " << url << " already exists!";
            return;
        }
    }

    Resource res;
    res.fullUrl = url;
    res.contentType = contentType;
    res.data = reply->peek(reply->bytesAvailable());
    m_resources.append(res);
    m_status.resourceCount += 1;
    m_status.resourceSuccess += (reply->error() == QNetworkReply::NoError ? 1 : 0);
}

void MHTSaver::ParseAndAddHTMLResources(QNetworkReply* reply)
{
    QUrl url = reply->url();
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    QByteArray data = reply->readAll();
    QString str = QString::fromUtf8(data);

    //Regular expressions stolen from the following and modified a bit:
    //http://www.codeproject.com/Articles/8268/Convert-any-URL-to-a-MHTML-archive-using-native-NE

    //This will also catch the (i)frames recursively, as we automatically load all htmls' resources.
    QString pattern = "(\\s(src|background)\\s*=\\s*|<link[^>]+?href\\s*=\\s*)"
                      "(?|'(?<url>[^'\\n\\r]+)|\"(?<url>[^\"\\n\\r]+)|(?<url>[^ \\n\\r]+))";
    QRegularExpression regexp(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = regexp.globalMatch(str);

    qDebug() << "PARSE HTML: Base: " << url;
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(url, linkedUrl);
    }

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
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    QByteArray data = reply->readAll();
    QString str = QString::fromUtf8(data);

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

    qDebug() << "PARSE CSS: Base: " << url;
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(url, linkedUrl);
    }
    while (matches2.hasNext())
    {
        QRegularExpressionMatch match = matches2.next();
        QString linkedUrl = match.captured("url");
        DecideAndLoadURL(url, linkedUrl);
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
        int indexOfLastSlash = basePath.lastIndexOf('/');
        QString basePathParent = (indexOfLastSlash == -1 ? QString() : basePath.left(indexOfLastSlash));
        finalURL = baseURL.scheme() + "://" + baseURL.authority()
                 + basePathParent + "/" + linkedURL;
    }

    qDebug() << "DECIDE: " << linkedURL << " => " << finalURL;
    LoadResource(QUrl(finalURL));
}

void MHTSaver::GenerateMHT()
{
    const QByteArray boundary = "----=_NextPart_000_00";

    QByteArray mhtdata;
    mhtdata += "From: <Saved by BookmarkManager>\r\n";

    Util::QEncodingOptions qenc;
    qenc.QEncoding = true;
    qenc.firstLineAdditionalLen = 9;
    qenc.originalEncoding = "utf-8";
    QByteArray subjectQEncoded = Util::EncodeQuotedPrintable(m_status.mainResourceTitle, false, qenc);
    mhtdata += "Subject: " + subjectQEncoded + "\r\n";

    QLocale usLocale(QLocale::English, QLocale::UnitedStates);
    QString currDateStr = usLocale.toString(QDateTime::currentDateTime(), ("ddd, dd MMM yyyy HH:mm:ss t"));
    mhtdata += "Date: " + currDateStr.toLatin1() + "\r\n";

    mhtdata += "MIME-Version: 1.0\r\n";

    QString firstResourceContentType = m_resources[0].contentType.split(';')[0].trimmed();
    mhtdata += "Content-Type: multipart/related;\r\n"
               "\ttype=\"" + firstResourceContentType.toLatin1() + "\";\r\n"
               "\tboundary=\"" + boundary + "\"\r\n";

    mhtdata += "X-Producer: BookmarkManager\r\n\r\n";

    mhtdata += "This is a multi-part message in MIME format.\r\n\r\n";

    foreach (const Resource& res, m_resources)
    {
        bool isText = isMimeTypeTextFile(res.contentType);

        mhtdata += "--" + boundary + "\r\n";

        if (!res.contentType.isEmpty())
            mhtdata += "Content-Type: " + res.contentType.toLatin1() + "\r\n";

        mhtdata += "Content-Transfer-Encoding: ";
        mhtdata += (isText ? "quoted-printable" : "base64");
        mhtdata += "\r\n";

        mhtdata += "Content-Location: " + res.fullUrl.toString(QUrl::FullyEncoded) + "\r\n";

        mhtdata += "\r\n";

        if (isText)
        {
            mhtdata += Util::EncodeQuotedPrintable(res.data);
        }
        else
        {
            QByteArray base64 = res.data.toBase64();
            QByteArray base64lines;
            for (int i = 0; i <= base64.length() / 76; i++)
                base64lines += base64.mid(i * 76, ((i+1)*76 > base64.length() ? -1 : 76)) + "\r\n";
            mhtdata += base64lines;
        }
        mhtdata += "\r\n";
    }

    //Last one has an additional '--' at end.
    mhtdata += "--" + boundary + "--\r\n";

    emit MHTDataReady(mhtdata, m_status);
}

bool MHTSaver::isMimeTypeTextFile(const QString& mimeType)
{
    QStringList applicationTextTypes = QStringList()
        << "application/atom+xml"
        << "application/ecmascript"
        << "application/json"
        << "application/javascript"
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
