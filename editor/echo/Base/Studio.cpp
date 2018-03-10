#include "Studio.h"
#include "MainWindow.h"
#include "ProjectWnd.h"
#include "RenderWindow.h"
#include "LogPanel.h"
#include "ConfigMgr.h"
#include <QtUiTools/QUiLoader>
#include <QFile>
#include <QLibrary>
#include <QMetaMethod>
#include <QSplitter>
#include "QPropertyModel.h"
#include "TimelinePanel.h"
#include <shellapi.h>
#include <engine/core/util/HashGenerator.h>
#include <engine/core/util/TimeProfiler.h>
#include <engine/core/util/PathUtil.h>
#include "EchoEngine.h"


namespace Studio
{
	// 构造函数
	AStudio::AStudio()
	{
		m_renderWindow = NULL;
		m_projectMgr = EchoNew( ProjectMgr);
		m_projectCfg = EchoNew( ConfigMgr);

		m_log = NULL;
	}

	AStudio::AStudio(const char* inputProject)
	{
		m_renderWindow = NULL;
		m_projectMgr = EchoNew( ProjectMgr);
		m_projectCfg = NULL;
		m_log = NULL;

		initLogSystem();
	}

	// 析构函数
	AStudio::~AStudio()
	{
		EchoRoot->destroy();
		EchoSafeDelete(m_logPanel, LogPanel);

		EchoSafeDelete(m_projectWindow, ProjectWnd);
		EchoSafeDelete(m_renderWindow, RenderWindow);
		EchoSafeDelete(m_projectMgr, ProjectMgr);
		EchoSafeDelete(m_projectCfg, ConfigMgr);
		EchoSafeDelete(m_mainWindow, MainWindow);

		// 打印内存泄漏日志
		Echo::MemoryManager::outputMemLeakInfo();
	}

	// 初始化日志系统
	bool AStudio::initLogSystem()
	{
		EchoNew(Echo::Root);
		EchoRoot->initLogSystem();

		// 添加默认日志处理
		Echo::LogDefault::LogConfig logConfig;
		logConfig.logName = "astudio.log";
		logConfig.logLevel = Echo::Log::LL_WARNING;
		logConfig.path = "./";
		logConfig.logFilename = "AStudio.log";
		logConfig.bFileOutput = true;

		EchoLogManager->setLogLeve(logConfig.logLevel);

		m_log = EchoNew(Echo::LogDefault(logConfig));
		if ( m_log )
			EchoLogManager->addLog(m_log);

		return true;
	}

	// 启动
	void AStudio::Start()
	{
		// 初始日志系统
		initLogSystem();

		// 启动主界面
		m_mainWindow = EchoNew( MainWindow);
		m_projectWindow = EchoNew( ProjectWnd);

		loadAllRecentProjects();

		// 启动日志面板
		m_logPanel = EchoNew(LogPanel( m_mainWindow));
		EchoLogManager->addLog(m_logPanel);
	}

	// 关闭
	void AStudio::Close()
	{

	}

	void AStudio::loadAllRecentProjects()
	{
		Echo::list<Echo::String>::type recentProjects;
		m_projectCfg->getAllRecentProject(recentProjects);

		Echo::list<Echo::String>::iterator iter = recentProjects.begin();
		for ( ; iter != recentProjects.end(); ++iter )
		{
			m_projectWindow->addRecentProject((*iter).c_str());
		}
	}

	// 判断缩略图是否存在
	bool AStudio::isThumbnailExists(const Echo::String& name)
	{
		Echo::String appPath = AStudio::Instance()->getAppPath();
		Echo::String fileFullName = Echo::StringUtil::Format("%sCache/thumbnail/%s.bmp", appPath, name);

		return Echo::PathUtil::IsFileExist(fileFullName);
	}

	bool AStudio::replaceTraverseAllWidget(QWidget* parent, QWidget* from, QWidget* to)
	{
		if ( parent && parent->layout() )
		{
			auto replaced = parent->layout()->replaceWidget(from, to);
			if ( replaced )
			{
				delete replaced;
				return true;
			}
			auto widgets = parent->findChildren<QWidget*>();
			if ( !widgets.size() )
			{
				return false;
			}
			for ( int i = 0; i < widgets.size(); ++i )
			{
				if ( replaceTraverseAllWidget(widgets[i], from, to) )
				{
					return true;
				}
			}
		}
		return false;
	}

	// 获取渲染窗口
	QWidget* AStudio::getRenderWindow()
	{
		// 新建渲染窗口
		if ( !m_renderWindow )
		{
			// 新建EchoEngine
			TIME_PROFILE
			(
				new EchoEngine;
				new ThumbnailMgr;
			)

			// 新建渲染窗口
			TIME_PROFILE
			(
				m_renderWindow = EchoNew(RenderWindow);
				m_renderWindow->BeginRender();
			)
		}

		return m_renderWindow;
	}

	// 设置渲染窗口控制器
	void AStudio::setRenderWindowController(IRWInputController* controller)
	{
		RenderWindow* renderWindow = qobject_cast<RenderWindow*>(m_renderWindow);
		if ( renderWindow )
		{
			renderWindow->setInputController(controller);
		}
	}

	IRWInputController* AStudio::getRenderWindowController()
	{
		RenderWindow* renderWindow = qobject_cast<RenderWindow*>(m_renderWindow);
		if (renderWindow)
		{
			return renderWindow->getInputController();
		}
		else
			return NULL;
	}

	// 获取主窗口
	QWidget* AStudio::getMainWindow()
	{
		return m_mainWindow;
	}

	QWidget* AStudio::getProjectWindow()
	{
		return m_projectWindow;
	}

	// 打开项目文件
	void AStudio::OpenProject(const char* fileName)
	{
		//生成缩略图
		ShellExecute(0, "open", "Thumbnail.exe", fileName, "", SW_HIDE);

		// 项目管理器
		TIME_PROFILE(m_projectMgr->OpenProject(fileName);)

		// 初始化渲染窗口
		TIME_PROFILE(EchoEngine::SetProject(fileName);)

		// 通知主窗口
		TIME_PROFILE
		(
			m_mainWindow->onOpenProject();
			m_mainWindow->setWindowTitle(fileName);
		)

		// 输出时间占比结果
		TIME_PROFILE_OUTPUT
	}

	// 设置程序工作路径
	void AStudio::setAppPath(const char* appPath)
	{
		m_appPath = appPath;
		Echo::PathUtil::FormatPath(m_appPath, true);
	}

	// 删除资源
	bool AStudio::deleteResource(const char* res)
	{

		return false;
	}

	// 资源是否可被删除
	bool AStudio::isResCanbeDeleted(const char* res)
	{
		return true;
	}

	// 保存缩略图
	bool AStudio::saveThumbnail(const Echo::String& fileName, int type /* = 0 */)
	{
		bool success = ThumbnailMgr::Instance()->saveThumbnail(fileName, ThumbnailMgr::THUMBNAIL_TYPE(type));
		if ( success )
		{
			QString itemName;
			itemName = fileName.c_str();
		}
		return success;
	}

	// 根据文件名获取缩略图全路径
	Echo::String AStudio::getThumbnailPath(const Echo::String& filePath, bool needOldExt)
	{
		// 过滤掉后缀名，加上bmp
		Echo::String fileName = Echo::PathUtil::GetPureFilename(filePath, needOldExt);
		Echo::String appPath = Echo::PathUtil::GetCurrentDir();

		unsigned int projectHash = Echo::BKDRHash(EchoRoot->getConfig().projectFile.c_str());
		Echo::String thumbnailPath = Echo::StringUtil::Format("%s/Cache/project_%d/thumbnail/%s.bmp", appPath.c_str(), projectHash, fileName.c_str());

		return thumbnailPath;
	}

	// 重置摄像机
	void AStudio::resetCamera(float diroffset)
	{
		auto* renderWindow = static_cast<RenderWindow*>(getRenderWindow());
		renderWindow->getInputController()->onInitCameraSettings(diroffset);
	}
}