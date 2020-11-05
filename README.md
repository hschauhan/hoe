[![Build Status](https://travis-ci.com/hschauhan/qemacs.svg?branch=main)](https://travis-ci.com/hschauhan/qemacs)

# Introduction

QEmacs (for Quick Emacs) is a very small but powerful UNIX editor. It has features that even big editors lack :

- Full screen editor with an Emacs look and feel with all Emacs common features: multi-buffer, multi-window, command mode, universal argument, keyboard macros, config file with C like syntax, minibuffer with completion and history.
- Can edit files of hundreds of Megabytes without being slow by using a highly optimized internal representation and by mmaping the file.
- Full UTF8 support, including bidirectional editing respecting the Unicode bidi algorithm. Arabic and Indic scripts handling (in progress).
- WYSIWYG HTML/XML/CSS2 mode graphical editing. Also supports lynx like rendering on VT100 terminals.
- WYSIWYG DocBook mode based on XML/CSS2 renderer.
- C mode: coloring with immediate update. Emacs like auto-indent.
- Shell mode: colorized VT100 emulation so that your shell work exactly as you expect. Compile mode with next/prev error.
- Input methods for most languages, including Chinese (input methods come from the Yudit editor).
- Hexadecimal editing mode with insertion and block commands. Unicode hexa editing of UTF8 files also supported.
- Works on any VT100 terminals without termcap. UTF8 VT100 support included with double width glyphs.
- X11 support. Support multiple proportionnal fonts at the same time (as XEmacs). X Input methods supported. Xft extension supported for anti aliased font display.
- Small! Full version (including HTML/XML/CSS2/DocBook rendering): 150KB big. Basic version (without bidir/unicode scripts/input/X11/C/Shell/HTML/dired): 49KB.

# Compiling
- If you want image, audio and video support, download FFmpeg at
  http://ffmpeg.org and install it in the qemacs/ directory (it should
  be in qemacs/ffmpeg).
- Launch the configure tool './configure'. You can look at the
  possible options by typing './configure --help'.
- Type 'make' to compile qemacs and its associated tools.
- type 'make install' as root to install it in /usr/local.

# Documentation
Read the file qe-doc.html.

# Licensing
QEmacs is released under the GNU Lesser General Public License (read
the accompagning COPYING file).

Fabrice Bellard

some plugins and fixes added by Himanshu Chauhan
