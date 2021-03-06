/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TEXTDOCUMENTLAYOUT_P_H
#define TEXTDOCUMENTLAYOUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qabstracttextdocumentlayout.h"
#include "QtGui/qtextoption.h"
#include "QtGui/qtextobject.h"
#include "lveditorglobal.h"

class QTextListFormat;

namespace lv {

class TextDocumentLayoutPrivate;
class LineManager;

class TextDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_DECLARE_PRIVATE(TextDocumentLayout)
    Q_OBJECT
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)
    Q_PROPERTY(qreal idealWidth READ idealWidth)
    Q_PROPERTY(bool contentHasAlignment READ contentHasAlignment)
public:
    explicit TextDocumentLayout(QTextDocument *doc);

    // from the abstract layout
    void draw(QPainter *painter, const PaintContext &context) override;
    int hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const override;

    int pageCount() const override;
    QSizeF documentSize() const override;

    void setCursorWidth(int width);
    int cursorWidth() const;

    // internal, to support the ugly FixedColumnWidth wordwrap mode in QTextEdit
    void setFixedColumnWidth(int width);

    // internal for QTextEdit's NoWrap mode
    void setViewport(const QRectF &viewport);

    virtual QRectF frameBoundingRect(QTextFrame *frame) const override;
    virtual QRectF blockBoundingRect(const QTextBlock &block) const override;

    // ####
    int layoutStatus() const;
    int dynamicPageCount() const;
    QSizeF dynamicDocumentSize() const;
    void ensureLayouted(qreal);

    qreal idealWidth() const;

    bool contentHasAlignment() const;

    LineManager* getLineManager();
    void stateChangeUpdate(int pos);


    // line manager wrappers
    QTextDocument* lineDocument();
    void collapseLines(int pos, int len);
    void expandLines(int pos, int len);
    std::pair<int,int> isFirstLineOfCollapsedSection(int lineNumber);
    std::pair<int,int> isLineAfterCollapsedSection(int lineNumber);
    QTextDocument* lineManagerParentDocument();
    void setLineManagerParentDocument(QTextDocument* doc);
    void setLineDocumentFont(const QFont& font);

    void setDirtyPos(int pos);
    void textDocumentFinishedUpdating(int newLineNumber);
    void updateLineSurface(int oldLineNum, int newLineNum, int dirtyPos);
Q_SIGNALS:
    void linesCollapsed(int pos, int len);
    void linesExpanded(int pos, int len);
    void updateLineSurfaceSignal(int oldLineNum, int newLineNum, int dirtyPos);
protected:
    void documentChanged(int from, int oldLength, int length) override;
    void resizeInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format) override;
    void positionInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format) override;
    void drawInlineObject(QPainter *p, const QRectF &rect, QTextInlineObject item,
                          int posInDocument, const QTextFormat &format) override;
    virtual void timerEvent(QTimerEvent *e) override;
private:
    QRectF doLayout(int from, int oldLength, int length);
    void layoutFinished();
};

}

#endif // TEXTDOCUMENTLAYOUT_P_H
