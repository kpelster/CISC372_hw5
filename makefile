
imageOMP:image_OpenMP.c image.h
	gcc -g -fopenmp image_OpenMP.c -o imageOMP -lm
imagePT:image_pthreads.c image.h
	gcc -g -lpthread image_pthreads.c -o imagePT -lm
image: image_OpenMP.c image_pthreads.c image.h
	gcc -g -fopenmp image_OpenMP.c -o imageOMP -lm	
	gcc -g -lpthread image_pthreads.c -o imagePT -lm
clean:
	rm -f imageOMP imagePT output.png