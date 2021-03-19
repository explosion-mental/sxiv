# sxiv
My build of sxiv
![Image](Screenshot.png "Screenshot")

## Key Handler
- With the patch I made (someone who knows diff and all that please make it correct), there's no need for "modkey" to execute a command on keyhandler, pretty stuff!
- thats it?

## Fork
I use someone's fork which I don't remember why I did it, but his fork I believe is on sxiv.1

## Patches
Just look them up, most of them aren't on this build.

## Help...
This is really the best image viewer and prob the fastest! After my changes it's even better for me. I have one issue
- What's the best position/intuitive (preferably not with Shift, my pinky hurts) for some actions on sxiv?
- Also, is there a way to maximaze space on thumbnail mode?(Maybe with square thumbnails dummy?)
- See *[TODO](TODO.md)* if you wanna help

## TODO (why have a separate file)
- Load all frames from TIFF files. We have to write our own loader for this to
  happen--just like we did for GIF images--because Imlib2 does not support
  multiple frames. Issue #241.
- Add support for more embedded thumbnail formats. Right now, sxiv seems to use
  the smallest one. Issue #238.

### My Build

- Autoreload feature  ( It needs to reload images when invoke keyhandler? the markcnt doesn't update after moving files to other directory with a keyhandler command )
- Use the space better
- '-T' for title and '-d' for dmenu like output?
- If the bar is hidden when image mode, when switching modes toggle the bar (maybe bar always shown on thumbnail mode? 	cg_toggle_bar();)
- after the padding patch I notice , I had an screenshot of my dwm bar it is really wide but not with that much height. When I mark it on thumbnail mode on some of the smalls zooms the image "dissapear" (the image was almost as thick as the marks)
- Is there a way to not navigate (to scroll) on WIDTH mode? (usefull on some memes, an comics)
- How to loop on the command 'cg_navigate_marked' ? (on marked images)
- <s>Actual support for clipboard, I use xsel just because clipmenu use it (I don't want to fork it now but some time I will in order to work with xclip)</s> I use xclip now 😁✅
