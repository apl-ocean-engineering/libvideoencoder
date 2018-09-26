# libvideoencoder


Side-by-side compositing with ffmpeg

    ffmpeg -i /tmp/test.mov -filter_complex "[v:0]pad=iw*2:ih:0[left];[left][v:1]overlay=W/2.0[fileout]" -map "[fileout]" -c:v prores /tmp/output.mov
