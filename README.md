# JC3636W518

## MJPEG convertion

[Video source](https://www.pexels.com/video/earth-rotating-video-856356/)
```console
 ffmpeg -i "Pexels Videos 3931.mp4" -ss 0 -t 20.4s -vf "reverse,setpts=0.5*PTS,fps=10,vflip,hflip,rotate=90,crop=720:720:178:598,scale=352:352:flags=lanczos" -q:v 9 earth352.mjpeg
```

[Video source](https://youtu.be/RpHnKaxt_OQ)
```console
ffmpeg -y -i "The Zoomquilt - an infinitely zooming collaborative painting.mp4" -ss 0 -t 00:02:00.000 -vf "fps=5,scale=-1:352:flags=lanczos,crop=352:352:(in_w-352)/2:0" -q:v 7 zoomquilt.mjpeg
```