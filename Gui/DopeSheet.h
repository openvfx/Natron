#ifndef DOPESHEET_H
#define DOPESHEET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h"

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

class DopeSheetPrivate;
class DopeSheetSelectionModel;
class DopeSheetSelectionModelPrivate;
class DSKnobPrivate;
class DSNodePrivate;
class Gui;
class HierarchyView;
class KnobI;
class KnobGui;
class NodeGroup;
class NodeGui;
class QUndoCommand;
class TimeLine;

namespace Natron {
class Node;
}

class DSKnob;
class DSNode;

typedef boost::shared_ptr<DSNode> DSNodePtr;
typedef boost::shared_ptr<DSKnob> DSKnobPtr;

typedef std::map<QTreeWidgetItem *, boost::shared_ptr<DSNode> > DSTreeItemNodeMap;
typedef std::map<QTreeWidgetItem *, boost::shared_ptr<DSKnob> > DSTreeItemKnobMap;
// typedefs

const int QT_ROLE_CONTEXT_TYPE = Qt::UserRole + 1;
const int QT_ROLE_CONTEXT_DIM = Qt::UserRole + 2;
const int QT_ROLE_CONTEXT_IS_ANIMATED = Qt::UserRole + 3;


bool nodeHasAnimation(const boost::shared_ptr<NodeGui> &nodeGui);


/**
 * @brief The DopeSheet class is the model part of the dope sheet editor.
 *
 * A DopeSheet is basically a bundle of "contexts". A context is a instance
 * of a class composed with a QTreeWidgetItem and a Natron data. Currently,
 * the class handles two types of contexts :
 *
 * - Node context : described by the DSNode class.
 * - Knob context : described by the DSKnob class.
 *
 * It provides several functions to find contexts and some of their datas.
 *
 *
 * --- User interactions ---
 *
 * The model handles each type of user interaction (that modify the model,
 * of course). For example, a specific function move the selected keyframes,
 * another move a reader...
 *
 * If you want to create a custom undoable modification, you must add a public
 * function that push a QUndoCommand-derived instance on the model's undo stack.
 */
class DopeSheet: public QObject
{
    Q_OBJECT

public:
    enum ItemType
    {
        ItemTypeCommon = 1001,

        // Range-based nodes
        ItemTypeReader,
        ItemTypeRetime,
        ItemTypeTimeOffset,
        ItemTypeFrameRange,
        ItemTypeGroup,

        // Others
        ItemTypeKnobRoot,
        ItemTypeKnobDim
    };

    DopeSheet(Gui *gui, const boost::shared_ptr<TimeLine> &timeline);
    ~DopeSheet();

    // Model specific
    DSTreeItemNodeMap getItemNodeMap() const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    boost::shared_ptr<DSNode> mapNameItemToDSNode(QTreeWidgetItem *nodeTreeItem) const;
    boost::shared_ptr<DSKnob> mapNameItemToDSKnob(QTreeWidgetItem *knobTreeItem) const;

    boost::shared_ptr<DSNode> findParentDSNode(QTreeWidgetItem *treeItem) const;
    boost::shared_ptr<DSNode> findDSNode(Natron::Node *node) const;
    boost::shared_ptr<DSNode> findDSNode(const boost::shared_ptr<KnobI> &knob) const;

    boost::shared_ptr<DSKnob> findDSKnob(KnobGui *knobGui) const;

    bool isPartOfGroup(DSNode *dsNode) const;
    boost::shared_ptr<DSNode> getGroupDSNode(DSNode *dsNode) const;

    std::vector<boost::shared_ptr<DSNode> > getImportantNodes(DSNode *dsNode) const;

    boost::shared_ptr<DSNode> getNearestTimeNodeFromOutputs(DSNode *dsNode) const;
    Natron::Node *getNearestReader(DSNode *timeNode) const;

    DopeSheetSelectionModel *getSelectionModel() const;

    // User interaction
    void deleteSelectedKeyframes();

    void moveSelectedKeys(double dt);
    void trimReaderLeft(const boost::shared_ptr<DSNode> &reader, double newFirstFrame);
    void trimReaderRight(const boost::shared_ptr<DSNode> &reader, double newLastFrame);
    void slipReader(const boost::shared_ptr<DSNode> &reader, double dt);
    void moveReader(const boost::shared_ptr<DSNode> &reader, double dt);
    void moveGroup(const boost::shared_ptr<DSNode> &group, double dt);
    void copySelectedKeys();
    void pasteKeys();
    void setSelectedKeysInterpolation(Natron::KeyframeTypeEnum keyType);

    // Undo/redo
    void setUndoStackActive();

    void emit_modelChanged();

    // Other
    SequenceTime getCurrentFrame() const;

Q_SIGNALS:
    void modelChanged();
    void nodeAdded(DSNode *dsNode);
    void nodeAboutToBeRemoved(DSNode *dsNode);
    void nodeRemoved(DSNode *dsNode);
    void keyframeSetOrRemoved(DSKnob *dsKnob);

private: /* functions */
    boost::shared_ptr<DSNode> createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, ItemType itemType);

private Q_SLOTS:
    void onNodeNameChanged(const QString &name);
    void onKeyframeSetOrRemoved();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};


/**
 * @brief The DSNode class describes a node context in the dope sheet.
 *
 * A node context is basically composed of a QTreeWidgetItem and a
 * NodeGui object.
 *
 * Use getTreeItem() to retrieve the tree widget item.
 *
 *
 * --- Retrieve node data ---
 *
 * A node context store a NodeGui object, which can be retrieved by using
 * getNodeGui().
 *
 * Use getInternalNode() to access to the Node object itself.
 *
 *
 * --- Child contexts ---
 *
 * According to the structure of a node, a DSNode stores a map composed of :
 * - a tree widget item named with a knob context text (knob name, dimension name).
 * - a DSKnob object (see below).
 *
 * Use getItemKnobMap() to get this map.
 *
 * The DSNode/DSKnob API is the dopesheet version of the Node/Knob organization.
 *
 *
 * -- Item types --
 *
 * Remember, the DopeSheet::ItemType enum describes each possible type for a
 * context.
 *
 * A DSNode stores this type because the dope sheet editor treats some types
 * of node differently. A DSNode can be a node exclusively composed of the
 * keyframes of its knobs. Another can be a reader node, which must be
 * drawn in a different way in the editor.
 *
 * Use getItemType() to retrieve this context type. Use isTimeNode() and
 * isRangeDrawingEnabled() to have more detailed infos about it.
 *
 * If you created a new node context type, please modify
 * isRangeDrawingEnabled() and canContainOtherNodeContexts() to handle it.
 *
 *
 * /!\ A better conception should remove the QTreeWidgetItem attribute
 * from this class.
 */
class DSNode
{
public:
    DSNode(DopeSheet *model,
           DopeSheet::ItemType itemType,
           const boost::shared_ptr<NodeGui> &nodeGui,
           QTreeWidgetItem *nameItem);
    ~DSNode();

    QTreeWidgetItem *getTreeItem() const;

    boost::shared_ptr<NodeGui> getNodeGui() const;
    boost::shared_ptr<Natron::Node> getInternalNode() const;

    DSTreeItemKnobMap getItemKnobMap() const;

    DopeSheet::ItemType getItemType() const;

    bool isTimeNode() const;

    bool isRangeDrawingEnabled() const;

    bool canContainOtherNodeContexts() const;
    bool containsNodeContext() const;

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};


/**
 * @brief The DSKnob class describes a knob context in the dope sheet.
 *
 * A knob context is basically composed of a QTreeWidgetItem and a
 * KnobGui object.
 *
 *
 * --- Simple and multidimensional knobs ---
 *
 * How the dope sheet treats the knobs of a node ?
 *
 * The main rule is : one knob context per dimension.
 *
 * According to Natron, a knob can have more than one dimension. In this case,
 * a DSKnob is created for each dimension and for the knob name.
 *
 * Example :
 * A knob named "Color" has three dimensions : r, g, b. The dope sheet will
 * treat it as :
 *
 * Color
 * - r
 * - g
 * - b
 *
 * So four contexts.
 *
 *
 * --- Dealing with dimensions ---
 *
 * A knob context also stores the dimension of the associated knob :
 * - 0 if the knob has one dimension.
 * - (N - X) if the knob has N dimensions. X refers to the current dimension.
 * - (-1) if the knob context is the root context of a multidim knob.
 *
 * getDimension() returns the dimension of the context.
 * isMuldiDimRoot() is a convenience function to check if the context is the
 * root of a multidim knob.
 *
 *
 * --- The QTreeWidgetItem-related API  ---
 *
 * Use getTreeItem() to retrieve the tree widget item.
 *
 * If 'this' is a knob root context and you want to obtain a pointer on the
 * tree widget item associated with a X dimension, use findDimTreeItem().
 *
 *
 * --- Retrieve knob data ---
 *
 * A knob context store a Knob object and the associated KnobGui object.
 *
 * Use getKnobGui() and getInternalKnob() to access to them.
 *
 *
 * /!\ A better conception should remove the QTreeWidgetItem attribute
 * from this class.
 */
class DSKnob
{
public:
    DSKnob(int dimension,
           QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QTreeWidgetItem *getTreeItem() const;
    QTreeWidgetItem *findDimTreeItem(int dimension) const;

    KnobGui *getKnobGui() const;
    boost::shared_ptr<KnobI> getInternalKnob() const;

    bool isMultiDimRoot() const;
    int getDimension() const;

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

/**
 * @brief The DopeSheetKey struct describes a keyframe in a knob context.
 *
 * It is and should be used only to handle keyframe selections.
 */
struct DopeSheetKey
{
    DopeSheetKey(const boost::shared_ptr<DSKnob> &knob, KeyFrame kf) :
        context(knob),
        key(kf)
    {
        boost::shared_ptr<DSKnob> knobContext = context.lock();
        assert(knobContext);

        assert(knobContext->getDimension() != -1);
    }

    DopeSheetKey(const DopeSheetKey &other) :
        context(other.context),
        key(other.key)
    {}

    friend bool operator==(const DopeSheetKey &key, const DopeSheetKey &other)
    {
        boost::shared_ptr<DSKnob> knobContext = key.context.lock();
        boost::shared_ptr<DSKnob> otherKnobContext = other.context.lock();
        if (!knobContext || !otherKnobContext) {
            return false;
        }

        if (knobContext != otherKnobContext) {
            return false;
        }

        if (key.key.getTime() != other.key.getTime()) {
            return false;
        }

        if (knobContext->getTreeItem() != otherKnobContext->getTreeItem()) {
            return false;
        }

        if (knobContext->getDimension() != otherKnobContext->getDimension()) {
            return false;
        }

        return true;
    }

    boost::shared_ptr<DSKnob> getContext() const
    {
        boost::shared_ptr<DSKnob> ret = context.lock();
        assert(ret);

        return ret;
    }

    boost::weak_ptr<DSKnob> context;
    KeyFrame key;
};

typedef boost::shared_ptr<DopeSheetKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;


/**
 * @brief The DopeSheetSelectionModel class
 *
 *
 */
class DopeSheetSelectionModel : public QObject
{
    Q_OBJECT

public:
    enum SelectionType {
        SelectionTypeNoSelection = 0x0,
        SelectionTypeClear = 0x1,
        SelectionTypeAdd = 0x2,
        SelectionTypeToggle = 0x4
    };

    Q_DECLARE_FLAGS(SelectionTypeFlags, SelectionType)

    DopeSheetSelectionModel(DopeSheet *dopeSheet);
    ~DopeSheetSelectionModel();

    void selectAllKeyframes();
    void selectKeyframes(const boost::shared_ptr<DSKnob> &dsKnob, std::vector<DopeSheetKey> *result);

    void clearKeyframeSelection();
    void makeSelection(const std::vector<DopeSheetKey> &keys, DopeSheetSelectionModel::SelectionTypeFlags selectionFlags);

    bool isEmpty() const;

    DSKeyPtrList getSelectedKeyframes() const;
    std::vector<DopeSheetKey> getSelectionCopy() const;

    int getSelectedKeyframesCount() const;

    bool keyframeIsSelected(const boost::shared_ptr<DSKnob> &dsKnob, const KeyFrame &keyframe) const;
    DSKeyPtrList::iterator keyframeIsSelected(const DopeSheetKey &key) const;

    void emit_keyframeSelectionChanged();

    void onNodeAboutToBeRemoved(const boost::shared_ptr<DSNode> &removed);

Q_SIGNALS:
    void keyframeSelectionChangedFromModel();

private:
    boost::scoped_ptr<DopeSheetSelectionModelPrivate> _imp;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DopeSheetSelectionModel::SelectionTypeFlags)

#endif // DOPESHEET_H
