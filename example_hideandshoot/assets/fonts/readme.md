Font folder
-------------
This folder contains the fonts used in game, note that since I explicitly want a low-resolution aliasing font I have created a custom 5 by 9 monospaced aliasing font as a png image, titled NineFive. It is primarily Serif because I like that, though wheverer the glyph appeared to cluttered I removed the serif.

NineFive supports the LIMITED EXTENDED LATIN (Basic, Supplement, Extension A and B) script Unicode 0x0000 to 0x024F. This should cover most currently used continental European languages, Pinyin (an  alternative phonetic writing system for Mandarin Chinese) and some African Latin-based scripts. Some extinct languages should also be covered by this

Furthermore NineFive provides LIMITED newspeak pictograms, or whatever the kids call them these days, (Unicode 0x1F300-1F5FF) (I will not add more, since especially the newer pictograms do not fit into 9 by 5)


I sincerely apologize for not adding support for more languages, but manually drawing these glyphs takes a long time,  and most non-Latin scripts won't even fit into the five by nine format. Still, with a larger resolution than nine-by five it should be possible to add support for, for example Arabic and Devanagari.

Normally, I (when not working with pixelated displays) recommend using a third party TTF font.

This folder contains a file "default.txt" which tells the program what font to load. The font folders must contain a file "caligraphy.txt"

caligraphy.txt, and modding fonts
===========
caligraphy.txt contains the following information on a line-per-line basis, and in this order:

At the very start of the file is the vertical spacing between lines in pixels

The next information can be repeated however many times you like to add in multiple different Unicode blocks (say you want to support Arabic and Runic but nothing between in Unicode)

The information for each Unicode block is:

png image used (No you do not need either black or white pixels, this is an artistic choice, though the program will interpret "darker" as more transparent), minimum unicode character (integer base 16), maximum unicode character (integer base 16), number of columns (integer base 10) and number of rows (integer base 10), then a list of pairs of integers telling how many pixels some numbers should be dropped below the baseline, for instance 42 2 tells the editor to drop the 42nd entry 2 pixels down, etc.

Then follows the horizontal spacing between characters (preferably 1 for latin based characters, but can be dropped to 0 for writing systems with one connecting line)

The final bit tells is 0 or 1 tells the program whether (1) or not (0) to use monospacedspacing.

NOTE 1 FOR MODDING NON-MONOSPACING FONTS: Each glyph must be placed on a rectangular grid, place the glyph as far left (<-) as possible and the program will automatically figure out the width. Just start by trying to mod the provided font.

Multiple similar entries can be used to represent multiple segments of Unicode, as an example see NineFive.

If the png image has width not divisible by the rows/columns it is simply cropped until it is (As is the case with NineFive)

Some people make a huge fuss about making sure that every texture you ever load has a width and height which is a power of two (2,4,8,16,32,64,128,512,1024,2048,4096 and so on), this is technically true, openGL is optimized for that, and if the font is not that size OpenGL may, behind the scene, try to upscale it until it is that size. This does add some performance penalties, but for a modern computer displaying a highly pixelated game or program this is not a big problem, and the internal texture loading functions have been tested with non-power of two width and height, including odd prime numbers as width and height.
