#pragma once
#include <QObject>

#include <QUrl>
#include <QHash>
#include <QDateTime>
#include <QStringList>

//TODO:
//- Redirect depth check
//- google sprite problem

class QTimer;
class QNetworkReply;
class QNetworkAccessManager;

/// Save URLs as MHT files!
///     This class is designed to suffice NOT-SO-IMPORTANT usages. It may not be suitable for
///     exact-accuracy saving needs.
///     It always produces an mhtml file data; i.e it even wraps images in this format.
/// Shortcomings:
///     1. Doesn't support any encoding other than UTF8
///     2. Doesn't smartly guess file types; just looks at their content type and file extension.
///        (partially does this actually, e.g if js/css files have text/html content type it
///        will fix it, but doesn't do the same thing for e.g images having text/html mime type.)
///        UPDATE: Now we use QMimeDatabase to correctly guess the mime type of unknown files on
///        based on extension and contents.
///     3. Doesn't have a timer or something to abort slow or stalled network operations.
///        We can use an e.g 20sec timer to abort operations with slow/stalled progresses during
///        the last 10 seconds. Update: But we DON'T. Maybe if we aren't receiving a qreply's
///        finished signal, we are just receiving a big file. And if it gets stalled for a long
///        time, we let the system time it out itself. Anyway if there is any such timer, it has to
///        record progress or number of received bytes and judge according to that, not time alone.
///        However that is for single replies alone. We do have a timer that controls overall
///        receiving time of the whole page. It should be set to a big value to allow for large file
///        downloads.
///     4. The HTML and CSS parsers are very simple, don't skip comments and use simple regexps.
///     5. Doesn't strip scripts.
///     6. Doesn't load resources that are additionally loaded in scripts, or change DOM after
///        loading a page; only a browser can do that.
///     7. Redirections support is limited and is only done correctly for the main page. Although
///        the redirect targets are being fetched and saved, they are NOT displayed e.g in case of
///        pictures, because we neither modify the urls in the html/css files nor do we save the
///        redirect locations under the old addresses. Ideally we had to do both, i.e convert the
///        redirect targets to some `urn:` form and use them.
///        UPDATE: We now replace the contents of the file with the old url with the contents and
///        content-type of the redirect target url. This doesn't require us to modify urls in the
///        source file. This works for all resources except the main resource, which uses its
///        original redirect handling.
///     8. I DID NOT GUARANTEE CORRECTNESS OF %-encoded URLS and other similar issues. I didn't
///        study QUrl<->QString conversions thoroughly about how to treat the encoding issues.
///     9. This is a feature, not a limitation: For single files, e.g image or pdf files, it will
///        create a raw file and will set the status.fileSuffix to the file extension.
class MHTSaver : public QObject
{
    Q_OBJECT

public:
    //If mainSuccess = true and mainHttpErrorCode = 0 this shows user cancelling the operation.
    struct Status
    {
        bool mainSuccess;
        int mainHttpErrorCode;
        int mainNetworkReplyError;
        QString mainNetworkReplyErrorString;
        QString mainResourceTitle;

        QString fileSuffix;
        int resourceCount;
        int resourceSuccess;
    };

private:
    QNetworkAccessManager* qnam;
    Status m_status;
    bool m_cancel;

    Q_PROPERTY(int overallTimeoutTime READ overallTimeoutTime WRITE setOverallTimeoutTime)
    bool m_useOverallTimer;
    QTimer* m_overallTimer;

    Q_PROPERTY(bool stripJS READ stripJS WRITE setStripJS)
    bool m_stripJS;

    struct Resource
    {
        QUrl fullUrl;
        QString contentType;
        QByteArray data;
        QUrl redirectTo;
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

    //// Properties ///////////////////////////////////////////////////////////
public:
    int overallTimeoutTime() const; ///Returns -1 if disabled
    bool stripJS() const;
public slots:
    void setOverallTimeoutTime(int seconds); ///Pass -1 to disable
    void setStripJS(bool strip);
private slots:
    void OverallTimerTimeout();

    //// Class Public Interface ///////////////////////////////////////////////
public slots:
    void GetMHTData(const QString& url);
    void Cancel();
signals:
    void MHTDataReady(const QByteArray& data, const MHTSaver::Status& status);

private:
    //// Loading Resources From Web ///////////////////////////////////////////
    void LoadResource(const QUrl& url);
    Q_SLOT void ResourceLoadingFinished();
    void DeleteReplyAndCheckForFinish(QNetworkReply* reply);
    QString AddResource(QNetworkReply* reply, const QString& redirectLocation);

    //// File Parsing /////////////////////////////////////////////////////////
    void ParseAndAddHTMLResources(QNetworkReply* reply);
    void ParseAndAddCSSResources(QNetworkReply* reply);
    void DecideAndLoadURL(const QUrl& baseURL, const QString& linkedURL);

    //// MHT Format Handling //////////////////////////////////////////////////
    // Have all resources, now generate the MHT file
    void GenerateMHT();
    // For single-file saves
    void GenerateFile();

    //// Utility Functions ////////////////////////////////////////////////////
    int findResourceWithURL(const QUrl& url);

    QString getRawContentType(const QString& contentType);
    bool isMimeTypeTextFile(const QString& mimeType);
    QString getOrGuessMimeType(QNetworkReply* reply, const QByteArray& data);

    QByteArray stripHTMLScripts(const QByteArray& html);
};
