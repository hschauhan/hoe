[![Build Status](https://travis-ci.com/hschauhan/hoe.svg?branch=main)](https://travis-ci.com/hschauhan/hoe)

# HOE: Himanshu' Own Emacs

## Definition
<b>HOE: A tool with a thin flat blade on a long handle used especially for cultivating,
weeding, or loosening the earth around plants.</b>

![Hoe](https://upload.wikimedia.org/wikipedia/commons/thumb/f/f1/Hoe%2C_toothed_Zierbena_CoA.svg/291px-Hoe%2C_toothed_Zierbena_CoA.svg.png)

Hoe helps in cultivating, weeding and nurturing the software code. :)

## Software
Hoe is a clone of Qemacs but diverges in philosophy. Qemacs does a lot of things that
an editor, IMHO, should not do. So Hoe is here only to edit code.

Hoe is based on v0.3.3 of Qemacs. At this version there is no active development going on with Qemacs.
Hoe strips off the following from Qemacs:
* Video player
* X and Windows support (Only TTY)
* HTML, CSS & Docbook
* html2png
* Unicode

So, Hoe is a bare-minimum emacs-like editor. But adds following capabilities to stripped Qemacs:
* Cscope support just like xcscope.el in Emacs (Same key bindings if config.eg is used)
* Rectangular operations like insert/delete are added.
* Command to delete trailing white spaces

# Compiling
There is nothing to configure on Hoe. Just do "make".

# Licensing
QEmacs is released under the GNU Lesser General Public License so is Hoe.
(readthe accompagning COPYING file).
