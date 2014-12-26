//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_TEXTRENDERER_H_
#define NATRON_GUI_TEXTRENDERER_H_
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
class QString;
class QColor;
class QFont;

namespace Natron {
class TextRenderer
{
public:

    TextRenderer();

    ~TextRenderer();

    void renderText(float x, float y, const QString &text, const QColor &color, const QFont &font) const;

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;
};
}

#endif // NATRON_GUI_TEXTRENDERER_H_
