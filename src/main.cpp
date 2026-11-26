#include <QApplication>
#include <filesystem>

#include "MainWindow.h"
#include "Logger.h"
#include "Config.h"

#ifdef _WIN32
#include "HardwareDetect_Win.h"
#else
#include "HardwareDetect_Linux.h"
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    auvio::Config::instance().load();

    const auto logPath = std::filesystem::path(auvio::Config::defaultFilePath()).parent_path() / "auvio.log";
    auvio::Logger::instance().setLogFile(logPath.string());
    auvio::Logger::instance().setMinLevel(auvio::LogLevel::Info);

    const auto hardware = auvio::HardwareDetector::detect();
    AUVIO_LOG_INFO << "CPU cores: " << hardware.cpuCoreCount;

    auvio::MainWindow window;
    window.show();

    return app.exec();
}
