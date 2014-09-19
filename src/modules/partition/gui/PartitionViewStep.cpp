/* === This file is part of Calamares - <http://github.com/calamares> ===
 *
 *   Copyright 2014, Aurélien Gâteau <agateau@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gui/PartitionViewStep.h>

#include <core/DeviceModel.h>
#include <core/PartitionCoreModule.h>
#include <core/PartitionModel.h>
#include <gui/ChoicePage.h>
#include <gui/EraseDiskPage.h>
#include <gui/AlongsidePage.h>
#include <gui/PartitionPage.h>
#include <gui/PartitionPreview.h>

#include "CalamaresVersion.h"
#include "utils/CalamaresUtilsGui.h"
#include "utils/Logger.h"
#include "widgets/WaitingWidget.h"

// Qt
#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QProcess>
#include <QStackedWidget>
#include <QTimer>

PartitionViewStep::PartitionViewStep( QObject* parent )
    : Calamares::ViewStep( parent )
    , m_widget( new QStackedWidget() )
    , m_core( new PartitionCoreModule( this ) )
    , m_choicePage( new ChoicePage() )
    , m_erasePage( new EraseDiskPage() )
    , m_alongsidePage( new AlongsidePage() )
    , m_manualPartitionPage( new PartitionPage( m_core ) )
{
    m_widget->setContentsMargins( 0, 0, 0, 0 );

    QWidget* waitingWidget = new WaitingWidget( tr( "Gathering system information..." ) );
    m_widget->addWidget( waitingWidget );

    QTimer* timer = new QTimer;
    timer->setSingleShot( true );
    connect( timer, &QTimer::timeout,
             [=]()
    {
        QString osproberOutput;
        QProcess osprober;
        osprober.setProgram( "os-prober" );
        osprober.setProcessChannelMode( QProcess::SeparateChannels );
        osprober.start();
        if ( !osprober.waitForStarted() )
        {
            cDebug() << "ERROR: os-prober cannot start.";
        }
        else if ( !osprober.waitForFinished( 60000 ) )
        {
            cDebug() << "ERROR: os-prober timed out.";
        }
        else
        {
            osproberOutput.append(
                QString::fromLocal8Bit(
                    osprober.readAllStandardOutput() ).trimmed() );
        }

        QStringList osproberLines;
        foreach ( const QString& line, osproberOutput.split( '\n' ) )
        {
            if ( !line.simplified().isEmpty() )
                osproberLines.append( line );
        }

        m_choicePage->init( m_core, osproberLines );
        m_erasePage->init( m_core );
        m_alongsidePage->init( m_core, osproberLines );

        m_widget->addWidget( m_choicePage );
        m_widget->addWidget( m_manualPartitionPage );
        m_widget->addWidget( m_erasePage );
        m_widget->removeWidget( waitingWidget );
        waitingWidget->deleteLater();

        timer->deleteLater();
    } );
    timer->start( 0 );

    connect( m_core,            &PartitionCoreModule::hasRootMountPointChanged,
             this,              &PartitionViewStep::nextStatusChanged );
    connect( m_choicePage,      &ChoicePage::nextStatusChanged,
             this,              &PartitionViewStep::nextStatusChanged );
    connect( m_erasePage,       &EraseDiskPage::nextStatusChanged,
             this,              &PartitionViewStep::nextStatusChanged );
    connect( m_alongsidePage,   &AlongsidePage::nextStatusChanged,
             this,              &PartitionViewStep::nextStatusChanged );
}


PartitionViewStep::~PartitionViewStep()
{
    if ( m_choicePage && m_choicePage->parent() == nullptr )
        m_choicePage->deleteLater();
    if ( m_manualPartitionPage && m_manualPartitionPage->parent() == nullptr )
        m_manualPartitionPage->deleteLater();
}


QString
PartitionViewStep::prettyName() const
{
    return tr( "Partitions" );
}


QWidget*
PartitionViewStep::widget()
{
    return m_widget;
}


QWidget*
PartitionViewStep::createSummaryWidget() const
{
    QWidget* widget = new QWidget;
    QFormLayout* layout = new QFormLayout( widget );
    layout->setMargin( 0 );

    QList< PartitionCoreModule::SummaryInfo > list = m_core->createSummaryInfo();
    for ( const auto& info : list )
    {
        PartitionPreview* preview;

        layout->addRow( new QLabel( info.deviceName ) );

        preview = new PartitionPreview;
        preview->setModel( info.partitionModelBefore );
        info.partitionModelBefore->setParent( widget );
        layout->addRow( tr( "Before:" ), preview );

        preview = new PartitionPreview;
        preview->setModel( info.partitionModelAfter );
        info.partitionModelAfter->setParent( widget );
        layout->addRow( tr( "After:" ), preview );
    }
    return widget;
}


void
PartitionViewStep::next()
{
    if ( m_choicePage == m_widget->currentWidget() )
    {
        if ( m_choicePage->currentChoice() == ChoicePage::Manual )
            m_widget->setCurrentWidget( m_manualPartitionPage );
        else if ( m_choicePage->currentChoice() == ChoicePage::Erase )
        {
            if ( m_core->isDirty() )
                m_core->revert();
            m_widget->setCurrentWidget( m_erasePage );
        }
        else if ( m_choicePage->currentChoice() == ChoicePage::Alongside )
        {
            if ( m_core->isDirty() )
                m_core->revert();
            m_widget->setCurrentWidget( m_alongsidePage );
        }
        cDebug() << "Choice applied: " << m_choicePage->currentChoice();
    }
    else
        emit done();
}


void
PartitionViewStep::back()
{
    if ( m_widget->currentWidget() != m_choicePage )
        m_widget->setCurrentWidget( m_choicePage );
}


bool
PartitionViewStep::isNextEnabled() const
{
    if ( m_choicePage && m_choicePage == m_widget->currentWidget() )
        return m_choicePage->isNextEnabled();

    if ( m_erasePage && m_erasePage == m_widget->currentWidget() )
    {
        return m_erasePage->isNextEnabled() &&
               m_core->hasRootMountPoint();
    }

    if ( m_manualPartitionPage && m_manualPartitionPage == m_widget->currentWidget() )
        return m_core->hasRootMountPoint();

    return false;
}


bool
PartitionViewStep::isAtBeginning() const
{
    if ( m_widget->currentWidget() == m_manualPartitionPage ||
         m_widget->currentWidget() == m_erasePage )
        return false;
    return true;
}


bool
PartitionViewStep::isAtEnd() const
{
    if ( m_choicePage == m_widget->currentWidget() )
        return false;
    return true;
}


QList< Calamares::job_ptr >
PartitionViewStep::jobs() const
{
    return m_core->jobs();
}
