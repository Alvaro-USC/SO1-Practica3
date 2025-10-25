#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>


int n;
double ultimo_valor;
/* Manejador de señales para el Hijo */
void manejador(int sig) 
{
    if (sig == SIGUSR1) 
    {
        printf("\n\n[PROCESO TERMINADO] Señal SIGUSR1 recibida. Finalizando...\n");
        exit(0);
    }
    else if (sig == SIGALRM) 
    {
        printf("Último valor calculado (n=%d): %.10f\n", n, ultimo_valor);
        alarm(1);
    }
}

int main(int argc, char** argv)
{
    signal(SIGALRM, manejador);
    signal(SIGUSR1, manejador);
    printf("PID del proceso: %d. Para terminar, usa 'kill -SIGUSR1 %d' en otra terminal.\n", getpid(), getpid());
    alarm(1);  
    if (argc > 2) 
    {
        n = atoi(argv[2]);
    }
    else n = 1;
    while(1)
    {
        ++n;
        ultimo_valor = pow(n, 0.5);
    }

    return 0;
}
