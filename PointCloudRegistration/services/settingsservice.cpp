#include "settingsservice.h"

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
{
    m_qsettings = new QSettings("PointCloudRegistration", "Settings", this);
    load();
}

SettingsService::~SettingsService()
{
    save();
}

void SettingsService::load()
{
    m_qsettings->beginGroup("ICP");
    m_settings.icpParams.maxIterations = m_qsettings->value("maxIterations", 50).toInt();
    m_settings.icpParams.tolerance = m_qsettings->value("tolerance", 1e-6).toDouble();
    m_settings.icpParams.sigmaMultiplier = m_qsettings->value("sigmaMultiplier", 3.0).toDouble();
    m_settings.icpParams.octreeMaxPoints = m_qsettings->value("octreeMaxPoints", 10).toInt();
    m_settings.icpParams.octreeMaxDepth = m_qsettings->value("octreeMaxDepth", 20).toInt();
    m_qsettings->endGroup();
    
    m_qsettings->beginGroup("Display");
    m_settings.sourcePointSize = m_qsettings->value("sourcePointSize", 2.0).toFloat();
    m_settings.targetPointSize = m_qsettings->value("targetPointSize", 2.0).toFloat();
    m_settings.sourceColor = m_qsettings->value("sourceColor", QColor(255, 100, 100)).value<QColor>();
    m_settings.targetColor = m_qsettings->value("targetColor", QColor(100, 100, 255)).value<QColor>();
    m_settings.showGrid = m_qsettings->value("showGrid", true).toBool();
    m_settings.smoothRendering = m_qsettings->value("smoothRendering", true).toBool();
    m_qsettings->endGroup();
    
    m_qsettings->beginGroup("Window");
    m_settings.followSystemTheme = m_qsettings->value("followSystemTheme", true).toBool();
    m_settings.preferDarkMode = m_qsettings->value("preferDarkMode", true).toBool();
    m_settings.restoreLastSession = m_qsettings->value("restoreLastSession", true).toBool();
    m_qsettings->endGroup();
}

void SettingsService::save()
{
    m_qsettings->beginGroup("ICP");
    m_qsettings->setValue("maxIterations", m_settings.icpParams.maxIterations);
    m_qsettings->setValue("tolerance", m_settings.icpParams.tolerance);
    m_qsettings->setValue("sigmaMultiplier", m_settings.icpParams.sigmaMultiplier);
    m_qsettings->setValue("octreeMaxPoints", m_settings.icpParams.octreeMaxPoints);
    m_qsettings->setValue("octreeMaxDepth", m_settings.icpParams.octreeMaxDepth);
    m_qsettings->endGroup();
    
    m_qsettings->beginGroup("Display");
    m_qsettings->setValue("sourcePointSize", m_settings.sourcePointSize);
    m_qsettings->setValue("targetPointSize", m_settings.targetPointSize);
    m_qsettings->setValue("sourceColor", m_settings.sourceColor);
    m_qsettings->setValue("targetColor", m_settings.targetColor);
    m_qsettings->setValue("showGrid", m_settings.showGrid);
    m_qsettings->setValue("smoothRendering", m_settings.smoothRendering);
    m_qsettings->endGroup();
    
    m_qsettings->beginGroup("Window");
    m_qsettings->setValue("followSystemTheme", m_settings.followSystemTheme);
    m_qsettings->setValue("preferDarkMode", m_settings.preferDarkMode);
    m_qsettings->setValue("restoreLastSession", m_settings.restoreLastSession);
    m_qsettings->endGroup();
    
    m_qsettings->sync();
}

void SettingsService::setSettings(const AppSettings& settings)
{
    m_settings = settings;
    save();
    emit settingsChanged(m_settings);
}

void SettingsService::setICPParameters(const ICPParameters& params)
{
    m_settings.icpParams = params;
    save();
    emit icpParametersChanged(params);
}
