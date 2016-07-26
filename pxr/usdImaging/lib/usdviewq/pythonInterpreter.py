#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
from pxr import Tf

from PySide import QtGui, QtCore

from code import InteractiveInterpreter
import os, sys, keyword

# just a handy debugging method
def _PrintToErr(line):
    old = sys.stdout
    sys.stdout = sys.__stderr__
    print line 
    sys.stdout = old
    
def _Redirected(method):
    def new(self, *args, **kw):
        old = sys.stdin, sys.stdout, sys.stderr
        sys.stdin, sys.stdout, sys.stderr = self, self, self
        try:
            ret = method(self, *args, **kw)
        finally:
            sys.stdin, sys.stdout, sys.stderr = old
        return ret
    return new

class _Completer(object):
    """Taken from rlcompleter, with readline references stripped, and a local
    dictionary to use."""
    
    def __init__(self, locals):
        self.locals = locals
        
    def Complete(self, text, state):
        """Return the next possible completion for 'text'.
        This is called successively with state == 0, 1, 2, ... until it
        returns None.  The completion should begin with 'text'.
        """
        if state == 0:
            if "." in text:
                self.matches = self._AttrMatches(text)
            else:
                self.matches = self._GlobalMatches(text)
        try:
            return self.matches[state]
        except IndexError:
            return None

    def _GlobalMatches(self, text):
        """Compute matches when text is a simple name.
        
        Return a list of all keywords, built-in functions and names
        currently defines in __main__ that match.
        """
        import __builtin__, __main__
        matches = set()
        n = len(text)
        for l in [keyword.kwlist,__builtin__.__dict__.keys(),
                  __main__.__dict__.keys(), self.locals.keys()]:
            for word in l:
                if word[:n] == text and word != "__builtins__":
                    matches.add(word)
        return list(matches)

    def _AttrMatches(self, text):
        """Compute matches when text contains a dot.
        
        Assuming the text is of the form NAME.NAME....[NAME], and is
        evaluatable in the globals of __main__, it will be evaluated
        and its attributes (as revealed by dir()) are used as possible
        completions.  (For class instances, class members are are also
        considered.)
        
        WARNING: this can still invoke arbitrary C code, if an object
        with a __getattr__ hook is evaluated.
        """
        import re, __main__

        assert len(text)

        # This is all a bit hacky, but that's tab-completion for you.

        # Now find the last index in the text of a set of characters, and split
        # the string into a prefix and suffix token there.  The suffix token
        # will be used for completion.
        splitChars = ' )(;,+=*/-%!<>'
        index = -1
        for char in splitChars:
            index = max(text.rfind(char), index)
            
        if index >= len(text)-1:
            return []

        prefix = ''
        suffix = text
        if index >= 0:
            prefix = text[:index+1]
            suffix = text[index+1:]

        m = re.match(r"([^.]+(\.[^.]+)*)\.(.*)", suffix)
        if not m:
            return []
        expr, attr = m.group(1, 3)
    
        try:
            myobject = eval(expr, __main__.__dict__, self.locals)
        except (AttributeError, NameError, SyntaxError):
            return []

        words = set(dir(myobject))
        if hasattr(myobject,'__class__'):
            words.add('__class__')
        words = words.union(set(_GetClassMembers(myobject.__class__)))

        words = list(words)
        matches = set()
        n = len(attr)
        for word in words:
            if word[:n] == attr and word != "__builtins__":
                matches.add("%s%s.%s" % (prefix, expr, word))
        return list(matches)

def _GetClassMembers(cls):
    ret = dir(cls)
    if hasattr(cls, '__bases__'):
        for base in cls.__bases__:
            ret = ret + _GetClassMembers(base)
    return ret
  

class Interpreter(InteractiveInterpreter):
    def __init__(self, widget, locals = None):
        InteractiveInterpreter.__init__(self,locals)
        self._widget = widget
        self._outputBrush = None

    # overridden
    def showsyntaxerror(self, filename = None):
        self._outputBrush = QtGui.QBrush(QtGui.QColor('#ffcc63'))

        try:
            InteractiveInterpreter.showsyntaxerror(self, filename)
        finally:
            self._outputBrush = None

    # overridden
    def showtraceback(self):
        self._outputBrush = QtGui.QBrush(QtGui.QColor('#ff0000'))

        try:
            InteractiveInterpreter.showtraceback(self)
        finally:
            self._outputBrush = None

    def GetOutputBrush(self):
        return self._outputBrush

# Modified from site.py in the Python distribution.
#
# This allows each interpreter editor to have it's own Helper object.
# The built-in pydoc.help grabs sys.stdin and sys.stdout the first time it
# is run and then never lets them go.
class _Helper(object):
    """Define a replacement for the built-in 'help'.
    This is a wrapper around pydoc.Helper (with a twist).

    """
    def __init__(self, input, output):
        import pydoc
        self._helper = pydoc.Helper(input, output)

    def __repr__(self):
        return "Type help() for interactive help, " \
               "or help(object) for help about object."

    def __call__(self, *args, **kwds):
        return self._helper(*args, **kwds)


class Controller(QtCore.QObject):
    """
    Controller is a Python shell written using Qt.

    This class is a controller between Python and something which acts
    like a QTextEdit.

    """

    _isAnyReadlineEventLoopActive = False

    def __init__(self, textEdit, initialPrompt, locals = None):
        """Constructor.

        The optional 'locals' argument specifies the dictionary in
        which code will be executed; it defaults to a newly created
        dictionary with key "__name__" set to "__console__" and key
        "__doc__" set to None.

        """

        QtCore.QObject.__init__(self)

        self.interpreter = Interpreter(textEdit, locals)
        self.interpreter.locals['help'] = _Helper(self, self)

        self.completer = _Completer(self.interpreter.locals)
        # last line + last incomplete lines
        self.lines = []

        # flag: the interpreter needs more input to run the last lines. 
        self.more = 0

        # history
        self.history = []
        self.historyPointer = None
        self.historyInput = ''

        # flag: readline() is being used for e.g. raw_input and input().
        # We use a nested QEventloop here because we want to emulate
        # modeless UI even though the readline protocol requires blocking calls.
        self.readlineEventLoop = QtCore.QEventLoop(textEdit)

        # interpreter prompt.
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "
            
        self.textEdit = textEdit
        self.connect(self.textEdit, QtCore.SIGNAL('destroyed()'),
                     self._TextEditDestroyedSlot)

        self.connect(self.textEdit, QtCore.SIGNAL("returnPressed()"),
                     self._ReturnPressedSlot)

        self.connect(self.textEdit, QtCore.SIGNAL("requestComplete()"),
                     self._CompleteSlot)

        self.connect(self.textEdit, QtCore.SIGNAL("requestNext()"),
                     self._NextSlot)

        self.connect(self.textEdit, QtCore.SIGNAL("requestPrev()"),
                     self._PrevSlot)

        appInstance = QtGui.QApplication.instance()
        self.connect(appInstance,
                     QtCore.SIGNAL("appControllerQuit()"),
                     self._QuitSlot)
        
        self.textEdit.setTabChangesFocus(False)

        self.textEdit.setWordWrapMode(QtGui.QTextOption.WrapAnywhere)
        self.textEdit.setWindowTitle('Interpreter')
        
        self.textEdit.promptLength = len(sys.ps1)

        # Do initial auto-import.
        self._DoAutoImports()

        # interpreter banner
        self.write('Python %s on %s.\n' % (sys.version, sys.platform))

        # Run $PYTHONSTARTUP startup script.
        startupFile = os.getenv('PYTHONSTARTUP')
        if startupFile:
            path = os.path.realpath(os.path.expanduser(startupFile))
            if os.path.isfile(path):
                self.ExecStartupFile(path)

        self.write(initialPrompt)
        self.write(sys.ps1)
        self.SetInputStart()

    def _DoAutoImports(self):
        modules = Tf.ScriptModuleLoader().GetModulesDict()
        for name, mod in modules.items():
            self.interpreter.runsource('import ' + mod.__name__ + 
                                       ' as ' + name + '\n')

    @_Redirected
    def ExecStartupFile(self, path):
        # fix for bug 9104
        # this sets __file__ in the globals dict while we are execing these
        # various startup scripts, so that they can access the location from
        # which they are being run.
        # also, update the globals dict after we exec the file (bug 9529)
        self.interpreter.runsource( 'g = dict(globals()); g["__file__"] = ' +
                                    '"%s"; execfile("%s", g);' % (path, path) +
                                    'del g["__file__"]; globals().update(g);' )
        self.SetInputStart()
        self.lines = []

    def SetInputStart(self):
        cursor = self.textEdit.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.textEdit.SetStartOfInput(cursor.position())

    def _QuitSlot(self):
        if self.readlineEventLoop:
            if self.readlineEventLoop.isRunning():
                self.readlineEventLoop.Exit()

    def _TextEditDestroyedSlot(self):
        self.readlineEventLoop = None

    def _ReturnPressedSlot(self):   
        if self.readlineEventLoop.isRunning():
            self.readlineEventLoop.Exit()
        else:
            self._Run()

    def flush(self):
        """
        Simulate stdin, stdout, and stderr.
        """
        pass

    def isatty(self):
        """
        Simulate stdin, stdout, and stderr.
        """
        return 1

    def readline(self):
        """
        Simulate stdin, stdout, and stderr.
        """
        # XXX: Prevent more than one interpreter from blocking on a readline()
        #      call.  Starting more than one subevent loop does not work,
        #      because they must exit in the order that they were created.
        if Controller._isAnyReadlineEventLoopActive:
            raise RuntimeError("Simultaneous readline() calls in multiple "
                               "interpreters are not supported.")

        cursor = self.textEdit.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.SetInputStart()
        self.textEdit.setTextCursor(cursor)

        try:
            Controller._isAnyReadlineEventLoopActive = True

            # XXX TODO - Make this suck less if possible.  We're invoking a
            # subeventloop here, which means until we return from this
            # readline we'll never get up to the main event loop.  To avoid
            # using a subeventloop, we would need to return control to the
            # main event loop in the main thread, suspending the execution of
            # the code that called into here, which also lives in the main
            # thread.  This essentially requires a
            # co-routine/continuation-based solution capable of dealing with
            # an arbitrary stack of interleaved Python/C calls (e.g. greenlet).
            self.readlineEventLoop.Exec()
        finally:
            Controller._isAnyReadlineEventLoopActive = False

        cursor.movePosition(QtGui.QTextCursor.EndOfBlock,
                            QtGui.QTextCursor.MoveAnchor)

        cursor.setPosition(self.textEdit.StartOfInput(),
                           QtGui.QTextCursor.KeepAnchor)
        txt = str(cursor.selectedText())

        if len(txt) == 0:
            return '\n'
        else:
            self.write('\n')
            return txt
            
    @_Redirected
    def write(self, text):
        'Simulate stdin, stdout, and stderr.'

        # Move the cursor to the end of the document
        self.textEdit.moveCursor(QtGui.QTextCursor.End)

        # Clear any existing text format.  We will explicitly set the format
        # later to something else if need be.
        self.textEdit.setCurrentCharFormat(QtGui.QTextCharFormat())

        # Copy the textEdit's current cursor.
        cursor = self.textEdit.textCursor()
        try:
            # If there's a designated output brush, merge that character format
            # into the cursor's character format.
            if self.interpreter.GetOutputBrush():
                cf = QtGui.QTextCharFormat()
                cf.setForeground(self.interpreter.GetOutputBrush())
                cursor.mergeCharFormat(cf)

            # Write the text to the textEdit.
            cursor.insertText(text)

        finally:
            # Set the textEdit's cursor to the end of input
            self.textEdit.moveCursor(QtGui.QTextCursor.End)

    # get the length of a string in pixels bases on our current font
    def _GetStringLengthInPixels(self, string):
        font = self.textEdit.font()
        fm = QtGui.QFontMetrics(font)
        strlen = fm.width(string)
        return strlen

    def _CompleteSlot(self):

        line = self._GetInputLine()
        cursor = self.textEdit.textCursor()
        origPos = cursor.position()
        cursor.setPosition(self.textEdit.StartOfInput(),
                           QtGui.QTextCursor.KeepAnchor)
        text = str(cursor.selectedText())
        tokens = text.split()
        token = ''
        if len(tokens) != 0:
            token = tokens[-1]

        completions = []
        p = self.completer.Complete(token,len(completions))
        while p != None:
            completions.append(p)
            p = self.completer.Complete(token, len(completions))

        if len(completions) == 0:
            return

        elif len(completions) != 1:
            self.write("\n")

            contentsRect = self.textEdit.contentsRect()
            # get the width inside the margins to eventually determine the
            # number of columns in our text table based on the max string width
            # of our completed words XXX TODO - paging based on widget height
            width = contentsRect.right() - contentsRect.left()
            
            maxLength = 0
            for i in completions:
                maxLength = max(maxLength, self._GetStringLengthInPixels(i))
            # pad it a bit
            maxLength = maxLength + self._GetStringLengthInPixels('  ')
                
            # how many columns can we fit on screen?
            numCols = max(1,width / maxLength)
            # how many rows do we need to fit our data
            numRows = (len(completions) / numCols) + 1

            columnWidth = QtGui.QTextLength(QtGui.QTextLength.FixedLength,
                                            maxLength)

            tableFormat = QtGui.QTextTableFormat()
            tableFormat.setAlignment(QtCore.Qt.AlignLeft)          
            tableFormat.setCellPadding(0)
            tableFormat.setCellSpacing(0)
            tableFormat.setColumnWidthConstraints([columnWidth] * numCols)
            tableFormat.setBorder(0)
            cursor = self.textEdit.textCursor()

            # Make the completion table insertion a single edit block
            cursor.beginEditBlock()
            cursor.movePosition(QtGui.QTextCursor.End) 
            textTable = cursor.insertTable(numRows, numCols, tableFormat)

            completions.sort()
            index = 0
            completionsLength = len(completions)

            for col in xrange(0,numCols):
                for row in xrange(0,numRows):
                    cellNum = (row * numCols) + col
                    if (cellNum >= completionsLength):
                        continue
                    tableCell = textTable.cellAt(row,col)
                    cellCursor = tableCell.firstCursorPosition()
                    cellCursor.insertText(completions[index])
                    index +=1

            cursor.endEditBlock()

            self.textEdit.setTextCursor(cursor)
            self.write("\n")
            if self.more:
                self.write(sys.ps2)
            else:
                self.write(sys.ps1)

            self.SetInputStart()
            # complete up to the common prefix
            cp = os.path.commonprefix(completions)

            # make sure that we keep everything after the cursor the same as it
            # was previously
            i = line.rfind(token)
            textToRight = line[i+len(token):]
            line = line[0:i] + cp + textToRight
            self.write(line)
          
            # replace the line and reset the cursor
            cursor = self.textEdit.textCursor()
            cursor.setPosition(self.textEdit.StartOfInput() + len(line) -
                               len(textToRight))

            self.textEdit.setTextCursor(cursor)

        else:
            i = line.rfind(token)
            line = line[0:i] + completions[0] + line[i+len(token):]

            # replace the line and reset the cursor
            cursor = self.textEdit.textCursor()

            cursor.setPosition(self.textEdit.StartOfInput(),
                               QtGui.QTextCursor.MoveAnchor)
                            
            cursor.movePosition(QtGui.QTextCursor.EndOfBlock,
                                QtGui.QTextCursor.KeepAnchor)

            cursor.removeSelectedText()
            cursor.insertText(line)
            cursor.setPosition(origPos + len(completions[0]) - len(token))
            self.textEdit.setTextCursor(cursor)

    def _NextSlot(self):
        if len(self.history):
            # if we have no history pointer, we can't go forward..
            if (self.historyPointer == None):
                return
            # if we are at the end of our history stack, we can't go forward
            elif (self.historyPointer == len(self.history) - 1):
                self._ClearLine()
                self.write(self.historyInput)
                self.historyPointer = None
                return
            self.historyPointer += 1
            self._Recall()
            
    def _PrevSlot(self):
        if len(self.history):
            # if we have no history pointer, set it to the most recent
            # item in the history stack, and stash away our current input
            if (self.historyPointer == None):
                self.historyPointer = len(self.history)
                self.historyInput = self._GetInputLine()
            # if we are at the end of our history, beep
            elif (self.historyPointer <= 0):
                return
            self.historyPointer -= 1
            self._Recall()

    def _IsBlank(self, txt):
        return len(txt.strip()) == 0

    def _GetInputLine(self):
        cursor = self.textEdit.textCursor()
        cursor.setPosition(self.textEdit.StartOfInput(),
                           QtGui.QTextCursor.MoveAnchor)
        cursor.movePosition(QtGui.QTextCursor.EndOfBlock,
                            QtGui.QTextCursor.KeepAnchor)
        txt = str(cursor.selectedText())
        return txt
        
    def _ClearLine(self):
        cursor = self.textEdit.textCursor()

        cursor.setPosition(self.textEdit.StartOfInput(),
                           QtGui.QTextCursor.MoveAnchor)
                            
        cursor.movePosition(QtGui.QTextCursor.EndOfBlock,
                            QtGui.QTextCursor.KeepAnchor)

        cursor.removeSelectedText()
        
    @_Redirected
    def _Run(self):
        """
        Append the last line to the history list, let the interpreter execute
        the last line(s), and clean up accounting for the interpreter results:
        (1) the interpreter succeeds
        (2) the interpreter fails, finds no errors and wants more line(s)
        (3) the interpreter fails, finds errors and writes them to sys.stderr
        """
        self.historyPointer = None
        inputLine = self._GetInputLine()
        if (inputLine != ""):
            self.history.append(inputLine)
            
        self.lines.append(inputLine)
        source = '\n'.join(self.lines)
        self.write('\n')
        self.more = self.interpreter.runsource(source)
        if self.more:
            self.write(sys.ps2)
            self.SetInputStart()
        else:
            self.write(sys.ps1)
            self.SetInputStart()
            self.lines = []
      
    def _Recall(self):
        """
        Display the current item from the command history.
        """
        self._ClearLine()
        self.write(self.history[self.historyPointer])

class View(QtGui.QTextEdit):
    """View is a QTextEdit which provides some extra
    facilities to help implement an interpreter console.  In particular,
    QTextEdit does not provide for complete control over the buffer being
    edited.  Some signals are emitted *after* action has already been
    taken, disallowing controller classes from really controlling the widget.
    This widget fixes that.
    """

    def __init__(self, parent=None):
        self.Parent = QtGui.QTextEdit
        self.Parent.__init__(self, parent)
        self.promptLength = 0
        self.__startOfInput = 0
        self.setUndoRedoEnabled(False)
        self.setAcceptRichText(False)
        self.setContextMenuPolicy(QtCore.Qt.NoContextMenu)
        self.tripleClickTimer = QtCore.QBasicTimer()
        self.tripleClickPoint = QtCore.QPoint()
        self._ignoreKeyPresses = True
        
    def SetStartOfInput(self, position):
        self.__startOfInput = position
        
    def StartOfInput(self):
        return self.__startOfInput

    def _PositionInInputArea(self, position):
        return position - self.__startOfInput

    def _PositionIsInInputArea(self, position):
        return self._PositionInInputArea(position) >= 0

    def _CursorIsInInputArea(self):
        return self._PositionIsInInputArea(self.textCursor().position())

    def _SelectionIsInInputArea(self):
        if (not self.textCursor().hasSelection()):
            return False
        selStart = self.textCursor().selectionStart()
        selEnd = self.textCursor().selectionEnd()
        return self._PositionIsInInputArea(selStart) and \
               self._PositionIsInInputArea(selEnd)

    def _MoveCursorToStartOfInput(self, select=False):
        cursor = self.textCursor()
        anchor = QtGui.QTextCursor.MoveAnchor

        if (select):
            anchor = QtGui.QTextCursor.KeepAnchor

        cursor.movePosition(QtGui.QTextCursor.End, anchor)
                    
        cursor.setPosition(self.__startOfInput, anchor)

        self.setTextCursor(cursor)

    def _MoveCursorToEndOfInput(self, select=False):
        c = self.textCursor()
        anchor = QtGui.QTextCursor.MoveAnchor
        if (select):
            anchor = QtGui.QTextCursor.KeepAnchor
            
        c.movePosition(QtGui.QTextCursor.End, anchor)
        self.setTextCursor(c)
        
    def _WritableCharsToLeftOfCursor(self):
        return (self._PositionInInputArea(self.textCursor().position()) > 0)

    def mousePressEvent(self, e):
        app = QtGui.QApplication.instance()        

        # is this a triple click?        
        if ((e.button() & QtCore.Qt.LeftButton) and
             self.tripleClickTimer.isActive() and
             (e.globalPos() - self.tripleClickPoint).manhattanLength() < 
              app.startDragDistance() ):

            # instead of duplicating the triple click code completely, we just
            # pass it along. but we modify the selection that comes out of it
            # to exclude the prompt, if appropriate
            self.Parent.mousePressEvent(self, e)

            if (self._CursorIsInInputArea()):
                selStart = self.textCursor().selectionStart()
                selEnd = self.textCursor().selectionEnd()

                if (self._PositionInInputArea(selStart) < 0):
                    # remove selection up until start of input
                     self._MoveCursorToStartOfInput(False)
                     cursor = self.textCursor()
                     cursor.setPosition(selEnd, QtGui.QTextCursor.KeepAnchor)
                     self.setTextCursor(cursor)
        else:
            self.Parent.mousePressEvent(self, e)
        
    def mouseDoubleClickEvent(self, e):
        self.Parent.mouseDoubleClickEvent(self, e)
        app = QtGui.QApplication.instance()
        self.tripleClickTimer.start(app.doubleClickInterval(), self);
        # make a copy here, otherwise tripleClickPoint will always = globalPos
        self.tripleClickPoint = QtCore.QPoint(e.globalPos())

    def timerEvent(self, e):
        if (e.timerId() == self.tripleClickTimer.timerId()):
            self.tripleClickTimer.stop()
        else:
            self.Parent.timerEvent(self, e)

    def enterEvent(self, e):
        self._ignoreKeyPresses = False
        
    def leaveEvent(self, e):
        self._ignoreKeyPresses = True

    def dragEnterEvent(self, e):
        self._ignoreKeyPresses = False
        self.Parent.dragEnterEvent(self, e)
        
    def dragLeaveEvent(self, e):
        self._ignoreKeyPresses = True
        self.Parent.dragLeaveEvent(self, e)
        
    def keyPressEvent(self, e):
        """
        Handle user input a key at a time.
        """

        if (self._ignoreKeyPresses):
            e.ignore()
            return
            
        text = e.text()
        key = e.key()

        ctrl = e.modifiers() & QtCore.Qt.ControlModifier
        shift = e.modifiers() & QtCore.Qt.ShiftModifier
        alt = e.modifiers() & QtCore.Qt.AltModifier

        cursorInInput = self._CursorIsInInputArea()
        selectionInInput = self._SelectionIsInInputArea()
        hasSelection = self.textCursor().hasSelection()
        canBackspace = self._WritableCharsToLeftOfCursor()
        canEraseSelection = selectionInInput and cursorInInput
        if key == QtCore.Qt.Key_Backspace:
            if (canBackspace and not hasSelection) or canEraseSelection:
                self.Parent.keyPressEvent(self, e)
        elif key == QtCore.Qt.Key_Delete:
            if (cursorInInput and not hasSelection) or canEraseSelection:
                self.Parent.keyPressEvent(self, e)
        elif key == QtCore.Qt.Key_Left:
            pos = self._PositionInInputArea(self.textCursor().position())
            if pos == 0:
                e.ignore()
            else:
                self.Parent.keyPressEvent(self, e)
        elif key == QtCore.Qt.Key_Right:
            self.Parent.keyPressEvent(self, e)
        elif key == QtCore.Qt.Key_Return or key == QtCore.Qt.Key_Enter:
            # move cursor to end of line.
            # emit signal to tell controller enter was pressed.
            if not cursorInInput:
                self._MoveCursorToStartOfInput(False)
            cursor = self.textCursor()
            cursor.movePosition(QtGui.QTextCursor.EndOfBlock)
            self.setTextCursor(cursor)
            # emit returnPressed
            self.emit(QtCore.SIGNAL("returnPressed()"))

        elif (key == QtCore.Qt.Key_Up 
                or key == QtCore.Qt.Key_Down
                # support Ctrl+P and Ctrl+N for history 
                # navigation along with arrows
                or (ctrl and (key == QtCore.Qt.Key_P 
                              or key == QtCore.Qt.Key_N))
                # support Ctrl+E and Ctrl+A for terminal
                # style nav. to the ends of the line
                or (ctrl and (key == QtCore.Qt.Key_A
                              or key == QtCore.Qt.Key_E))):
            if cursorInInput:
                if (key == QtCore.Qt.Key_Up or key == QtCore.Qt.Key_P):
                    self.emit(QtCore.SIGNAL("requestPrev()"))
                if (key == QtCore.Qt.Key_Down or key == QtCore.Qt.Key_N):
                    self.emit(QtCore.SIGNAL("requestNext()"))
                if (key == QtCore.Qt.Key_A):
                    self._MoveCursorToStartOfInput(False)
                if (key == QtCore.Qt.Key_E):
                    self._MoveCursorToEndOfInput(False)
                e.ignore()
            else:
                self.Parent.keyPressEvent(self, e)
        elif key == QtCore.Qt.Key_Tab:
            self.AutoComplete()
            e.accept()
        elif (ctrl or alt or 
              key == QtCore.Qt.Key_Home or 
              key == QtCore.Qt.Key_End):
            # Ignore built-in QTextEdit hotkeys so we can handle them with
            # our App-level hotkey system.
            e.ignore()
        elif hasSelection and not selectionInInput:
            # if we have some stuff other than our input line selected,
            # just deselect and append keypresses
            cursor = self.textCursor()
            self._MoveCursorToEndOfInput()
            self.Parent.keyPressEvent(self, e)
        elif not cursorInInput:
            # Ignore keypresses if we're not in the input area.
            e.ignore()
        else:
            self.Parent.keyPressEvent(self, e)

    def AutoComplete(self):
        if self._CursorIsInInputArea():
            self.emit(QtCore.SIGNAL("requestComplete()"))

    def _MoveCursorToBeginning(self, select=False):
        if self._CursorIsInInputArea():
            self._MoveCursorToStartOfInput(select)
        else:
            cursor = self.textCursor()
            anchor = QtGui.QTextCursor.MoveAnchor
            if (select):
                anchor = QtGui.QTextCursor.KeepAnchor
            cursor.setPosition(0, anchor)
            self.setTextCursor(cursor)

    def _MoveCursorToEnd(self, select=False):
        if self._CursorIsInInputArea():
            self._MoveCursorToEndOfInput(select)
        else:
            cursor = self.textCursor()
            anchor = QtGui.QTextCursor.MoveAnchor
            if (select):
                anchor = QtGui.QTextCursor.KeepAnchor

            cursor.setPosition(self.__startOfInput, anchor)
            cursor.movePosition(QtGui.QTextCursor.Up, anchor)
            cursor.movePosition(QtGui.QTextCursor.EndOfLine, anchor)
            self.setTextCursor(cursor)
            
    def MoveCursorToBeginning(self):
        self._MoveCursorToBeginning(False)
            
    def MoveCursorToEnd(self):
        self._MoveCursorToEnd(False)
        
    def SelectToTop(self):
        self._MoveCursorToBeginning(True)
        
    def SelectToBottom(self):
        self._MoveCursorToEnd(True)
