/* === This file is part of Calamares - <http://github.com/calamares> ===
 *
 *   Copyright 2015, Anke Boersma <demm@kaosx.us>
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

#ifndef LICENSEPAGE_H
#define LICENSEPAGE_H

#include <QWidget>

namespace Ui
{
class LicensePage;
}

class LicensePage : public QWidget
{
    Q_OBJECT
public:
    explicit LicensePage( QWidget* parent = nullptr );
    
    void showNvidiaUrl( bool enabled );
    void showCatalystUrl( bool enabled );
    void showFlashUrl( bool enabled );
    void showLicenseUrl( bool enabled );

/*protected:
    void focusInEvent( QFocusEvent* e ) override; //choose the child widget to focus */

private:
    Ui::LicensePage* ui;
};

#endif //LICENSEPAGE_H