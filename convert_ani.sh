#!/bin/sh

do_file_ani () {
	fn=$1
	./convert_ani $fn
	rm -f $fn.mp4 $fn.gif
	# ffmpeg -framerate 20 -pattern_type glob -i '*.BMP' -c:v libx264 $fn.mp4
	convert -delay 5 -loop 0 -layers Optimize *BMP $fn.gif
	rm -f ANIM*.BMP
}

for f in $1/*.ANI; do
	do_file_ani $( basename $f )
done
