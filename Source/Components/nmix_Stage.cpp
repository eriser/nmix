/*
 *  Copyright 2016 Antonio Lassandro 
 *
 *  nmix is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nmix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "nmix_Stage.h"
#include "nmix_Application.h"
#include "nmix_Colours.h"
#include "nmix_CommandIDs.h"
#include "nmix_Node.h"

nmix::Stage::Stage()
{
    setColour(backgroundColourId, nmix::Colours::DarkerGrey);
    setColour(foregroundColourId, nmix::Colours::DarkGrey);
    
    setWantsKeyboardFocus(true);
    
    selectedNodes.addChangeListener(this);
    
    juce::ApplicationCommandManager& commandManager = nmix::Application::getCommandManager();
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());
    
    nmix::Node* n = new nmix::Node(*this);
    n->setBounds(32, 32, 32, 32);
    addAndMakeVisible(n);
    stagedNodes.add(n);
}

nmix::Stage::~Stage()
{
    stagedNodes.clear();
}

juce::ApplicationCommandTarget* nmix::Stage::getNextCommandTarget()
{
    return nullptr;
}

void nmix::Stage::getAllCommands(juce::Array<juce::CommandID> &commands)
{
    const juce::CommandID ids[] =
    {
        nmix::CommandIds::SelectAll,
        nmix::CommandIds::DeselectAll,
        
        nmix::CommandIds::NudgeSelection,
        nmix::CommandIds::LockSelection,
    };
    
    commands.addArray(ids, juce::numElementsInArray(ids));
}

void nmix::Stage::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result)
{
    switch (commandID) {
        case nmix::CommandIds::SelectAll:
            
            result.setInfo("Select All", "Select All Nodes", nmix::CommandCategories::Stage, 0);
            
            result.addDefaultKeypress('a', juce::ModifierKeys::commandModifier);
            
            break;
            
        case nmix::CommandIds::DeselectAll:
            
            result.setInfo("Deselect All", "Deselect All Nodes", nmix::CommandCategories::Stage, 0);
            
            result.addDefaultKeypress('d', juce::ModifierKeys::commandModifier);
            
            break;
            
            
        case nmix::CommandIds::NudgeSelection:
            
            result.setInfo("Nudge", "Nudge Selected Nodes", nmix::CommandCategories::Stage, 0);
            
            result.addDefaultKeypress(juce::KeyPress::upKey,    juce::ModifierKeys::noModifiers);
            result.addDefaultKeypress(juce::KeyPress::downKey,  juce::ModifierKeys::noModifiers);
            result.addDefaultKeypress(juce::KeyPress::leftKey,  juce::ModifierKeys::noModifiers);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers);
            
            result.addDefaultKeypress(juce::KeyPress::upKey,    juce::ModifierKeys::shiftModifier);
            result.addDefaultKeypress(juce::KeyPress::downKey,  juce::ModifierKeys::shiftModifier);
            result.addDefaultKeypress(juce::KeyPress::leftKey,  juce::ModifierKeys::shiftModifier);
            result.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier);
            
            break;
            
        case nmix::CommandIds::LockSelection:
            
            result.setInfo("Lock Selection", "Lock Selected Nodes", nmix::CommandCategories::Stage, 0);
            
            result.addDefaultKeypress('l', juce::ModifierKeys::commandModifier);
            
            break;
            
        default:
            break;
    }
}

bool nmix::Stage::perform(const juce::ApplicationCommandTarget::InvocationInfo &info)
{
    switch (info.commandID)
    {
        case nmix::CommandIds::SelectAll:
            
            for (nmix::Node** n = stagedNodes.begin(); n != stagedNodes.end(); ++n)
            {
                selectedNodes.addToSelection(*n);
            }
            
            break;
            
        case nmix::CommandIds::DeselectAll:
            
            selectedNodes.deselectAll();
            
            break;
            
        case nmix::CommandIds::NudgeSelection:
        {
            
            int nudgeValue = (info.keyPress.getModifiers().isShiftDown()) ? 10 : 1;
            
            int deltaX =
              (info.keyPress.getKeyCode() == juce::KeyPress::upKey)    ? 0
            : (info.keyPress.getKeyCode() == juce::KeyPress::downKey)  ? 0
            : (info.keyPress.getKeyCode() == juce::KeyPress::leftKey)  ? -nudgeValue
            : (info.keyPress.getKeyCode() == juce::KeyPress::rightKey) ? nudgeValue
            : 0;
            
            int deltaY =
              (info.keyPress.getKeyCode() == juce::KeyPress::upKey)    ? -nudgeValue
            : (info.keyPress.getKeyCode() == juce::KeyPress::downKey)  ? nudgeValue
            : (info.keyPress.getKeyCode() == juce::KeyPress::leftKey)  ? 0
            : (info.keyPress.getKeyCode() == juce::KeyPress::rightKey) ? 0
            : 0;
            
            for (nmix::Node** n = selectedNodes.begin(); n != selectedNodes.end(); ++n)
            {
                (*n)->setTopLeftPosition((*n)->getPosition().translated(deltaX, deltaY));
            }
            
            repaint();
            
            break;
        }
            
        case nmix::CommandIds::LockSelection:
            
            for (nmix::Node** n = selectedNodes.begin(); n != selectedNodes.end(); ++n)
            {
                (*n)->status ^= nmix::Node::Locked;
                (*n)->repaint();
            }
            
            break;
            
        default:
            return false;
    }
    
    return true;
}

void nmix::Stage::mouseDown(const juce::MouseEvent &e)
{
    addAndMakeVisible(lasso);
    lasso.beginLasso(e, this);
}

void nmix::Stage::mouseDrag(const juce::MouseEvent &e)
{
    if (e.mouseWasDraggedSinceMouseDown())
    {
        lasso.dragLasso(e);
        lasso.toFront(true);
    }
}

void nmix::Stage::mouseUp(const juce::MouseEvent &e)
{
    lasso.endLasso();
    removeChildComponent(&lasso);
    if (!e.mouseWasDraggedSinceMouseDown())
    {
        selectedNodes.deselectAll();
    }
}

void nmix::Stage::findLassoItemsInArea(juce::Array<nmix::Node *>& results, const juce::Rectangle<int> &area)
{
    for (nmix::Node** n = stagedNodes.begin(); n != stagedNodes.end(); ++n)
    {
        if ((*n)->getBounds().intersects(area))
        {
            results.add(*n);
        }
    }
}

juce::SelectedItemSet<nmix::Node*>& nmix::Stage::getLassoSelection()
{
    return selectedNodes;
}

void nmix::Stage::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    for (nmix::Node** n = stagedNodes.begin(); n != stagedNodes.end(); ++n)
    {
        if (selectedNodes.isSelected(*n))
        {
            (*n)->status |= nmix::Node::Selected;
        }
        else
        {
            (*n)->status &= ~nmix::Node::Selected;
        }
        
        (*n)->repaint();
        repaint();
    }
}

void nmix::Stage::paint(juce::Graphics& g)
{
    int w = getWidth();
    int h = getHeight();
    
    g.fillAll(findColour(backgroundColourId));
    
    g.setColour(findColour(foregroundColourId));
    g.drawVerticalLine  (w/2, 0, h);
    g.drawHorizontalLine(h/2, 0, w);
    
    for (int i = 0; i <= 3; i++)
    {
        g.drawEllipse((w/8)*i, (h/8)*i, w - (w/4 * i), h - (h/4 * i), 0.5f);
    }
    
    if (selectedNodes.getNumSelected() > 0)
    {
        for (nmix::Node** n = selectedNodes.begin(); n != selectedNodes.end(); ++n)
        {
            int nWidth  = (*n)->getWidth();
            int nHeight = (*n)->getHeight();
            
            int radius = (*n)->getPosition().translated(nWidth/2, nHeight/2).getDistanceFrom(juce::Point<int>(w/2, h/2));
            
            g.setColour((*n)->findColour(nmix::Node::backgroundColourId));
            g.drawEllipse((w/2) - radius, (h/2) - radius, radius*2, radius*2, 1);
            g.drawLine(w/2, h/2, (*n)->getX() + nWidth/2, (*n)->getY() + nHeight/2, 1);
        }
    }
}

void nmix::Stage::resized()
{
    
}