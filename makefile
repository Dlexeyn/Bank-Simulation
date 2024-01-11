all: bank_simulation

clean:
	rm -rf Bankir_Simulation *.o

bank_simulation:
	gcc -o bank_simulation Bankir_Simulation.c