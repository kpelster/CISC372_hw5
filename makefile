
imageOMP:image_OpenMP.c image_OMP.h
	gcc -g -fopenmp image_OpenMP.c -o imageOMP -lm
imagePT:image_pthreads.c image_PT.h
	gcc -g -lpthread image_pthreads.c -o imagePT -lm
image: image_OpenMP.c image_pthreads.c image_OMP.h image_PT.h
	gcc -g -fopenmp image_OpenMP.c -o imageOMP -lm	
	gcc -g -lpthread image_pthreads.c -o imagePT -lm
clean:
	rm -f imageOMP imagePT output.png