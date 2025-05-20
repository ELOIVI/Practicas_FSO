all : winsuport.o tron0 tron1 semafor.o missatge.o memoria.o winsuport2.o tron2 oponent3 tron3 tron4 oponent5 tron5 oponent6 tron6

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o
	
winsuport2.o : winsuport2.c winsuport2.h
	gcc -Wall -c winsuport2.c -o winsuport2.o
	
semafor.o : semafor.c semafor.h
	gcc -Wall -c semafor.c -o semafor.o
	
missatge.o : missatge.c missatge.h
	gcc -Wall -c missatge.c -o missatge.o
	
memoria.o : memoria.c memoria.h
	gcc -Wall -c memoria.c -o memoria.o

tron0 : tron0.c winsuport.o winsuport.h
	gcc -Wall tron0.c winsuport.o -o tron0 -lcurses

tron1 : tron1.c winsuport.o winsuport.h
	gcc -Wall tron1.c winsuport.o -o tron1 -lcurses
	
tron2 : tron2.c winsuport.o winsuport.h semafor.o semafor.h memoria.o memoria.h
	gcc -Wall tron2.c winsuport.o semafor.o memoria.o -o tron2 -lcurses
	
oponent3 : oponent3.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h
	gcc -Wall oponent3.c winsuport2.o semafor.o memoria.o -o oponent3 -lcurses
	
tron3 : tron3.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h
	gcc -Wall tron3.c winsuport2.o semafor.o memoria.o -o tron3 -lcurses
	
tron4 : tron4.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h
	gcc -Wall tron4.c winsuport2.o semafor.o memoria.o -o tron4 -lcurses

tron5 : tron5.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h missatge.o missatge.h
	gcc -Wall tron5.c winsuport2.o semafor.o memoria.o missatge.o -o tron5 -lcurses
	
oponent5 : oponent5.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h missatge.o missatge.h
	gcc -Wall oponent5.c winsuport2.o semafor.o memoria.o missatge.o -o oponent5 -lcurses
	
tron6 : tron6.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h missatge.o missatge.h
	gcc -Wall tron6.c winsuport2.o semafor.o memoria.o missatge.o -o tron6 -lcurses
	
oponent6 : oponent6.c winsuport2.o winsuport2.h semafor.o semafor.h memoria.o memoria.h missatge.o missatge.h
	gcc -Wall oponent6.c winsuport2.o semafor.o memoria.o missatge.o -o oponent6 -lcurses
	
clean: 
	rm winsuport.o winsuport2.o semafor.o missatge.o memoria.o tron0 tron1 tron2 tron3 oponent3 tron4 oponent5 tron5 oponent6 tron6
