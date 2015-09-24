#pragma once
#include <QObject>

#include <QUrl>
#include <QHash>
#include <QDateTime>
#include <QStringList>

class QNetworkReply;
class QNetworkAccessManager;

/// Save URLs as MHT files!
///     This class is designed to suffice NOT-SO-IMPORTANT usages. It may not be suitable for
///     exact-accuracy saving needs.
/// Shortcomings:
///     1. Doesn't support any encoding other than UTF8
///     2. Doesn't smartly guess file types; just looks at their content type and file extension.
///        (partially does this actually, e.g if js/css files have text/html content type it
///        will fix it, but doesn't do the same thing for e.g images having text/html mime type.)
///     3. Doesn't have a timer or something to abort slow or stalled network operations.
///     4. The HTML and CSS parsers are very simple, don't skip comments and use simple regexps.
///     5. Doesn't strip scripts.
///     6. Doesn't load resources that are additionally loaded in scripts; only a browser can
///        do that.
///     7. Redirections support is limited and is only done correctly for the main page. Although
///        the redirect targets are being fetched and saved, they are NOT displayed e.g in case of
///        pictures, because we neither modify the urls in the html/css files nor do we save the
///        redirect locations under the old addresses. Ideally we had to do both, i.e convert the
///        redirect targets to some `urn:` form and use them.
///     8. I DID NOT GUARANTEE CORRECTNESS OF %-encoded URLS and other similar issues. I didn't
///        study QUrl<->QString conversions thoroughly about how to treat the encoding issues.
class MHTSaver : public QObject
{
    Q_OBJECT

public:
    struct Status
    {
        bool mainSuccess;
        int mainHttpErrorCode;
        QByteArray mainResourceTitle;

        int resourceCount;
        int resourceSuccess;
    };

private:
    QNetworkAccessManager* qnam;
    Status m_status;

    struct Resource
    {
        QUrl fullUrl;
        QString contentType;
        QByteArray data;
    };
    QList<Resource> m_resources;

    struct OngoingReply
    {
        //Note: Can use a timer to abort operations with slow/stalled progresses.
        int progress;
        QDateTime progressTimeStamp;
    };
    QHash<QNetworkReply*,OngoingReply> m_ongoingReplies;

    QStringList m_htmlContentTypes;
    QStringList m_scriptContentTypes;
    QStringList m_cssContentTypes;
    QStringList m_otherKnownContentTypes;

public:
    explicit MHTSaver(QObject *parent = 0);
    ~MHTSaver();

    //// Class Public Interface ///////////////////////////////////////////////
public slots:
    void GetMHTData(const QString& url);
signals:
    void MHTDataReady(const QByteArray& data, const MHTSaver::Status& status);

private:
    //// Loading Resources From Web ///////////////////////////////////////////
    void LoadResource(const QUrl& url);
    Q_SLOT void ResourceLoadingFinished();
    void DeleteReplyAndCheckForFinish(QNetworkReply* reply);
    void AddResource(QNetworkReply* reply);

    //// File Parsing /////////////////////////////////////////////////////////
    void ParseAndAddHTMLResources(QNetworkReply* reply);
    void ParseAndAddCSSResources(QNetworkReply* reply);
    void DecideAndLoadURL(const QUrl& baseURL, const QString& linkedURL);

    //// MHT Format Handling //////////////////////////////////////////////////
    // Have all resources, now generate the MHT file
    void GenerateMHT();
    bool isMimeTypeTextFile(const QString& mimeType);
};
