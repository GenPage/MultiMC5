#include "QuickModIndexDownloadAction.h"

#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "logic/quickmod/QuickModIndexModel.h"

#include "MultiMC.h"
#include "logger/QsLog.h"
#include "logic/MMCJson.h"
#include "logic/net/NetJob.h"

QuickModIndexModel *QuickModIndexDownloadAction::m_indexList = nullptr;

QuickModIndexDownloadAction::QuickModIndexDownloadAction(const QUrl &url, NetJob *netjob)
	: QuickModBaseDownloadAction(url), m_job(netjob)
{
	if (!m_indexList)
	{
		m_indexList = new QuickModIndexModel;
	}
}

bool QuickModIndexDownloadAction::handle(const QByteArray &data)
{
	Q_ASSERT(m_job);
	try
	{
		const QJsonObject root = MMCJson::ensureObject(MMCJson::parseDocument(data, "QuickMod Index"));
		const QString repo = MMCJson::ensureString(root.value("repo"));
		const QString baseUrlString = MMCJson::ensureString(root.value("baseUrl"));

		// FIXME: ALWAYS use original url (not the one that has followed redirects)!
		// The alternative is to tell redirects apart.
		// furthest node in transitive closure of permanent moves from first URL can be used as a new URL
		m_indexList->setRepositoryIndexUrl(repo, m_url);

		const QJsonArray array = MMCJson::ensureArray(root.value("index"));
		for (const QJsonValue itemVal : array)
		{
			const QJsonObject itemObj = MMCJson::ensureObject(itemVal);
			const QString uid = MMCJson::ensureString(itemObj.value("uid"));
			if (!MMC->qmdb()->haveUid(QuickModRef(uid), repo))
			{
				const QString urlString = MMCJson::ensureString(itemObj.value("url"));
				QUrl url;
				if (baseUrlString.contains("{}"))
				{
					url = QUrl(QString(baseUrlString).replace("{}", urlString));
				}
				else
				{
					url = QUrl(baseUrlString).resolved(QUrl(urlString));
				}
				m_job->addNetAction(QuickModBaseDownloadAction::make(m_job, url, uid));
			}
		}
	}
	catch (MMCError &e)
	{
		m_errorString = e.cause();
		return false;
	}
	return true;
}