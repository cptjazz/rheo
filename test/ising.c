#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#define MEAS 3000
#define EQUI 500
#define SKIP 10
#define RUNLENGTH 20

// __expected:should_update(dE => $_retval, kt => $_retval)
char should_update(double dE, double kt) {		
        // __define:exp(0 => $_retval)
	double prob = exp(-dE/kt);
        // __define:rand()
	double r = (float)rand()/(float)RAND_MAX;
	
	return (prob > r);
}

// __expected:get_neighbour_indices_for(i => $_retval, l => $_retval, a => $_retval)
int get_neighbour_indices_for(int i, int l, int a) {
	int x = i % l;
	int y = i / l;

	switch(a) {
		case 0:
			// left
			if (x == 0)
				return i + l - 1;
			return i - 1;

		case 2:
			// right
			if (x == l - 1)
				return i - x;
			return i + 1;

		case 1:
			// top
			if (y == 0)
				return l * (l - 1) + x;
			return i - l;

		case 3:
			// bottom
			if (y == l - 1)
				return x;
			return i + l;
                default:
                        return 0;
	}
}

// __expected:get_neighbour_sum(neighbours => $_retval, i => $_retval)
short get_neighbour_sum(short *spins, short*** neighbours, int i, int l) {
	return *neighbours[i][0] + *neighbours[i][1] + *neighbours[i][2] + *neighbours[i][3];
}

// __expected:do_monte_carlo_update(l => spins, neighbours => spins, j => spins, h => spins, kT => spins)
void do_monte_carlo_update(short* spins, short*** neighbours, int l, double j, double h, double kT) {
	for (int i = 0; i < l * l; i++) {
		short sig = spins[i];
		short sum = get_neighbour_sum(spins, neighbours, i, l);
		
		double dE = 2.0 * sig * (j * sum + h);
		
		if (dE < 0 || should_update(dE, kT)) 
			spins[i] = -sig;
		
	}
}

// __expected:equilibrate(l => spins, neighbours => spins, j => spins, h => spins, kT => spins)
void equilibrate(short* spins, short*** neighbours, int l, double j, double h, double kT) {
	for (int i = 0; i < EQUI; i++)
		do_monte_carlo_update(spins, neighbours, l, j, h, kT);
}

// __expected:get_magnetization(spins => $_retval, l => $_retval)
double get_magnetization(short* spins, int l) {
	int sum = 0;
	
	for (int i = 0; i < l * l; i++)
		sum += spins[i];

        // __define:fabs(0 => $_retval)
	return fabs(sum)  / (l * l);
}

// __expected:get_energy(spins => $_retval, l => $_retval, j => $_retval, h => $_retval, neighbours => $_retval)
double get_energy(short* spins, short*** neighbours, int l, double j, double h) {
	double sum = 0;
	
	for (int i = 0; i < l * l; i++)
		sum -=  spins[i] * (j * get_neighbour_sum(spins, neighbours, i, l) + h);

	return sum  / (l * l);
}

// __expected:measure(l => spins, neighbours => spins, j => spins, h => spins, kT => spins, l => energy, neighbours => energy, j => energy, h => energy, kT => energy, spins => energy, l => energy2, neighbours => energy2, j => energy2, h => energy2, kT => energy2, spins => energy2, l => magnetization, neighbours => magnetization, j => magnetization, h => magnetization, kT => magnetization, spins => magnetization, l => magnetization2, neighbours => magnetization2, j => magnetization2, h => magnetization2, kT => magnetization2, spins => magnetization2)
void measure(short* spins, short*** neighbours, int l, double j, double h, double kT, double* energy, double* magnetization, double* energy2, double* magnetization2) {
	double e = 0.0;
	double e2 = 0.0;
	double m = 0.0;
	double m2 = 0.0;

	for (int i = 0; i < MEAS; i++)
	{
		for (int i = 0; i < SKIP; i++)
			do_monte_carlo_update(spins, neighbours, l, j, h, kT);

		//do_monte_carlo_update(spins, neighbours, l, j, h, kT);
		
		double result = get_energy(spins, neighbours, l, j, h);
		e += result;
		e2 += result * result;
		
		result = get_magnetization(spins, l);
		m += result;
		m2 += result * result;
	}
	
	e /= (double) MEAS;
	e2 /= (double) MEAS;
	m /= (double) MEAS;
	m2 /= (double) MEAS;
	
	(*energy) = e;
	(*energy2) = sqrt(e2 - e * e);
	(*magnetization) = m;
	(*magnetization2) = sqrt(m2 - m * m);
}

// __expected:create_lattice(l => spins)
void create_lattice(short* spins, int l) {
	for (int i = 0; i < l * l; i++) 
		spins[i] = (rand() % 2 > 0) ? 1 : -1;
}

// __expected:create_neighbours(l => neighbours, spins => neighbours)
void create_neighbours(short* spins, short*** neighbours, int l) {
	for (int i = 0; i < l * l; i++) {
		neighbours[i] = (short**) malloc(sizeof(short*) * 4);
		
		for (int j = 0; j < 4; j++)
			neighbours[i][j] = &spins[get_neighbour_indices_for(i, l, j)];
	}
}

// __expected:write_file()
void write_file(double j, double* kT, double* energies, double* magnetization, double* energies2, double* magnetization2, int l) {
	char filename[255];
	FILE *fp;
	
	sprintf(filename, "ising_l%d_j%2.1f.dat", l, j);
	fp = fopen(filename, "w");
	
	for(int i = 0; i < RUNLENGTH; i++)
		fprintf(fp, "%f %f %f %f %f\n", kT[i], energies[i], energies2[i], magnetization[i], magnetization2[i]);
	
	fclose(fp);
}

// __expected:run_ising()
void run_ising(double j, int l) {	
	short* spins = (short*) malloc( sizeof(short) * l * l);
	short*** neighbours = (short***) malloc( sizeof(short**) * l * l);
	
	double* energies = (double*) malloc (sizeof(double) * RUNLENGTH);
	double* magnetization = (double*) malloc (sizeof(double) * RUNLENGTH);
	double* energies2 = (double*) malloc (sizeof(double) * RUNLENGTH);
	double* magnetization2 = (double*) malloc (sizeof(double) * RUNLENGTH);
	double* kT = (double*) malloc(sizeof(double) * RUNLENGTH);
	
	double h = 0.0;
	
	// Init prng
	srand ( time(NULL) );
	
	create_neighbours(spins, neighbours, l);

	for (int i = 0; i < RUNLENGTH; i++) {
		kT[i] = ((i + 1.0) * 3.0) / (double)RUNLENGTH;
	
		// Initialize lattice
		create_lattice(spins, l);

		equilibrate(spins, neighbours, l, j, h, kT[i]);
		measure(spins, neighbours, l, j, h, kT[i], &energies[i], &magnetization[i], &energies2[i], &magnetization2[i]);
	}	
	
	write_file(j, kT, energies, magnetization, energies2, magnetization2, l);
	
	free(energies);
	free(magnetization);
	free(kT);
	free(spins);
	free(neighbours);
}
