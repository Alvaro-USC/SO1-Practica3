#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Función de utilidad para obtener la hora actual como string (para claridad del log)
char* get_time_str() {
    time_t now = time(NULL);
    struct tm* tm_struct = localtime(&now);
    static char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_struct);
    return time_str;
}

// Manejador de señales (para SIGUSR1 y SIGUSR2) ---
void manejador_sig(int signum) 
{
    if (signum == SIGUSR1) 
    {
        printf("\n[%s] PADRE (PID: %d): MANEJADOR. Señal SIGUSR1 procesada.\n", 
               get_time_str(), getpid());
    } else if (signum == SIGUSR2) 
    {
        printf("\n[%s] PADRE (PID: %d): MANEJADOR. Señal SIGUSR2 procesada.\n", 
               get_time_str(), getpid());
    }
}

int main() 
{
    pid_t P = getpid();
    pid_t H1, N1 = 0;
    int status;
    struct sigaction sa;
    sigset_t new_mask, pending_mask;
    
    // Configuración y Captura de Señales con sigaction
    printf("[%s] PADRE (PID: %d): Iniciando y configurando manejadores.\n", 
           get_time_str(), P);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejador_sig;
    sa.sa_flags = SA_RESTART; // Activación de SA_RESTART

    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }

    // Bloqueo de SIGUSR1 en el Padre (P)
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    
    printf("[%s] PADRE (PID: %d): Bloqueando la señal SIGUSR1 (sigprocmask)....\n", 
           get_time_str(), P);
    
    // SIG_BLOCK: Añadir new_mask a la máscara de señales bloqueadas
    if (sigprocmask(SIG_BLOCK, &new_mask, NULL) == -1) {
        perror("sigprocmask SIG_BLOCK error");
        exit(1);
    }
    
    // Creación de H1 (Hijo)
    H1 = fork();

    if (H1 == -1) {
        perror("fork H1 error");
        exit(1);
    }

    if (H1 == 0) {
        // HIJO H1
        pid_t H1_PID = getpid();
        pid_t P_PID = getppid();
        
        printf("[%s] HIJO H1 (PID: %d): Creado. Creando al nieto N1...\n", 
               get_time_str(), H1_PID);
        
        // Creación del Nieto N1
        N1 = fork();

        if (N1 == 0) {
            // NIETO N1
            pid_t N1_PID = getpid();
            
            printf("[%s] NIETO N1 (PID: %d): Enviando SIGUSR2 al abuelo P (%d).\n", 
                   get_time_str(), N1_PID, P_PID);
            
            // N1 envía SIGUSR2 al Abuelo P (Esta señal no está bloqueada)
            if (kill(P_PID, SIGUSR2) == -1) {
                perror("N1 kill SIGUSR2 error");
            }

            printf("[%s] NIETO N1 (PID: %d): Esperando 5 segundos y terminando...\n", 
                   get_time_str(), N1_PID);
            sleep(5); 
            
            printf("[%s] NIETO N1 (PID: %d): Terminando.\n", 
                   get_time_str(), N1_PID);
            
            exit(0); // Terminación limpia
        } else {
            // H1
            printf("[%s] HIJO H1 (PID: %d): Nieto N1 (%d) creado. Enviando SIGUSR1 al abuelo P (%d).\n", 
                   get_time_str(), H1_PID, N1, P_PID);

            // H1 envía SIGUSR1 al Abuelo P (Esta señal será bloqueada y quedará pendiente)
            if (kill(P_PID, SIGUSR1) == -1) {
                perror("H1 kill SIGUSR1 error");
            }
            
            printf("[%s] HIJO H1 (PID: %d): Señal SIGUSR1 enviada. Esperando a N1...\n", 
                   get_time_str(), H1_PID);
                   
            // H1 espera a que N1 termine
            waitpid(N1, &status, 0); 
            
            // H1 devuelve el PID del nieto. Usamos el código de salida, asumiendo que el PID es menor a 256.
            printf("[%s] HIJO H1 (PID: %d): Nieto N1 terminado. Devolviendo PID N1 (%d).\n", 
                   get_time_str(), H1_PID, N1);
            
            exit((int)N1); 
        }

    } else {
        // PADRE P
        
        // El Padre duerme para permitir que H1 envíe SIGUSR1 y N1 envíe SIGUSR2
        printf("[%s] PADRE (PID: %d): H1 (%d) creado. Durmiendo 7 segundos.\n", 
               get_time_str(), P, H1);
        sleep(7);
        
        // Comprobar señales pendientes (antes de desbloquear)
        printf("[%s] PADRE (PID: %d): Despertado. Comprobando señales pendientes...\n", 
               get_time_str(), P);

        if (sigpending(&pending_mask) == -1) {
            perror("sigpending error");
        } else {
            // sigismember comprueba si SIGUSR1 está en la máscara de pendientes
            if (sigismember(&pending_mask, SIGUSR1)) {
                printf("[%s] PADRE (PID: %d): SIGUSR1 está PENDIENTE. (Recibida de H1 mientras estaba bloqueada).\n", 
                       get_time_str(), P);
            } else {
                printf("[%s] PADRE (PID: %d): SIGUSR1 NO está pendiente.\n", 
                       get_time_str(), P);
            }
        }
        
        // Desbloquear SIGUSR1
        printf("[%s] PADRE (PID: %d): Desbloqueando SIGUSR1. El kernel procesará inmediatamente SIGUSR1 PENDIENTE.\n", 
               get_time_str(), P);

        if (sigprocmask(SIG_UNBLOCK, &new_mask, NULL) == -1) {
            perror("sigprocmask SIG_UNBLOCK error");
        }
        
        // Esperar a H1
        printf("[%s] PADRE (PID: %d): Esperando a que el Hijo H1 (%d) termine...\n", 
               get_time_str(), P, H1);
        
        pid_t terminated_pid = waitpid(H1, &status, 0);

        if (terminated_pid > 0 && WIFEXITED(status)) { //  WIFEXITED comprueba que el H1 acabó por un exit() 
            // El hijo devuelve el PID del nieto como código de salida en su exit(), eso se obtiene mediante WEXITSTATUS
            N1 = WEXITSTATUS(status);
            printf("[%s] PADRE (PID: %d): HIJO H1 (%d) terminado. Código del Nieto N1 devuelto: %d.\n", 
                   get_time_str(), P, terminated_pid, N1);
        } else {
             printf("[%s] PADRE (PID: %d): Error en waitpid o terminación anormal de H1.\n", 
                   get_time_str(), P);
        }

        printf("[%s] PADRE (PID: %d): Terminando normalmente.\n", get_time_str(), P);
    }

    return 0;
}