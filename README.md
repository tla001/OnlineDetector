#			Online Object Detection System Based On Deep Learning
	This project is a system for objects detection based on deep learning(darknet --yolo),and includes some parts:
## 1.video stream detect
	use ffmpeg,perform that:
*	 1) get video stream from camera/file/net video stream
*	 2) decode and display with opencv
*	 3) encode video with h264 in flv format
*	 4) push video through rtmp protocol to nginx stream media server or save as file

### the main work of ./client/ObjectDetector:
*	pusher:
		get video from camera-->decode and display with opencv -->encode in h264as flv --> push stream to nginx server
*	puller:
		get video from nginx server-->decode and display with opencv -->encode in h264as flv --> save as file

### the main work of ./server/video
	get video from nginx server--> decode and dislay with opencv --> detect whih yolo -->display detect results --> encode in h264 as flv --> push stream to nginx server(another channel)

## 2.image detect
	tranport image or json with http,using libevent to listen http in server,using libcurl as client

### the main work of ./client/imagedemo/curlclient
	read a image file --> transport in http with libcurl ----waiting for reback-- get json stream -->decode json

### the main work of ./server/image
	libevent as http server in child process--- wait for client --- get a request --> write image stream as file --> signal to main process and yolo read file and detect -->write detect results in a json file --> fifo to wake up child process --> read json file and sent to client with http


#CAUSTION:
	when use codes in server folder,you must configure darknet already,then you can put these files in src,and replace default Makefile with my Makefile.Also, you have to configure libs I have used.

#	Something maybe more clear in my blog:www.cnblogs.com/tla001
	
