#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =========================================================================
// 1. GESTIONE DELLO STATO DELLA TARTARUGA (STRUTTURA DATI STACK / PILA)
// =========================================================================
// Implementazione custom di una pila LIFO per preservare le coordinate 
// spaziali e l'orientamento angolare durante la ramificazione (branching).
// =========================================================================

typedef struct {
    float x, y, z;       
    float rotX, rotY;    
} TurtleState;

#define MAX_STACK 1000
TurtleState stack[MAX_STACK];
int stackPointer = -1;

/**
 * Operazione Push: Salva lo stato corrente della tartaruga sul top dello stack.
 * Necessaria all'incontro del modulo di ramificazione '['.
 */
void push(float x, float y, float z, float rx, float ry) {
    if (stackPointer < MAX_STACK - 1) {
        stackPointer++;
        stack[stackPointer].x = x;
        stack[stackPointer].y = y;
        stack[stackPointer].z = z;
        stack[stackPointer].rotX = rx;
        stack[stackPointer].rotY = ry;
    } else {
        fprintf(stderr, "Errore Critico: Stack Overflow!\n");
    }
}

/**
 * Operazione Pop: Ripristina l'ultimo stato valido dal top dello stack.
 * Invocata alla chiusura della ramificazione ']'.
 */
TurtleState pop() {
    if (stackPointer >= 0) {
        return stack[stackPointer--];
    }
    fprintf(stderr, "Errore Critico: Stack Underflow!\n");
    TurtleState empty = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    return empty;
}

// =========================================================================
// 2. MOTORE DI GRAMMATICA FORMALE (GENERATORE L-SYSTEM)
// =========================================================================
// Sezione dedicata alla riscrittura delle stringhe secondo regole di 
// produzione parallele deterministiche e context-free (OL-System).
// =========================================================================

/**
 * Applica le regole di produzione formale su base generazionale.
 * Trasforma ogni modulo 'F' espandendolo secondo la regola che viene immessa.
 */
char* applicaRegole(const char* input) {
    // Allocazione predittiva della memoria basata sul fattore di crescita teorico della stringa
    char* output = (char*)malloc(strlen(input) * 15 * sizeof(char));
    if (!output) {
        fprintf(stderr, "Errore: Memoria insufficiente!\n");
        exit(1);
    }
    output[0] = '\0';

    int i = 0;
    while (input[i] != '\0') {
        if (input[i] == 'F') {
            // Regola di riscrittura formale imposta dalla consegna
            // E' possibile testare altre regole se immesse nella riga di sotto.
            strcat(output, "FF[+F][-F]");
        } else {
            char str_speciale[2] = {input[i], '\0'};
            strcat(output, str_speciale);
        }
        i++;
    }
    return output;
}

/**
 * Funzione iterativa per la computazione dei passaggi generazionali del frattale.
 * Gestisce correttamente il ciclo di vita della memoria (free del livello precedente).
 */
char* generaLSystem(const char* assioma, int generazioni) {
    char* corrente = strdup(assioma);
    for (int i = 0; i < generazioni; i++) {
        char* prossima = applicaRegole(corrente);
        free(corrente); 
        corrente = prossima;
    }
    return corrente;
}

// =========================================================================
// 3. INTERPRETE GRAFICO (TURTLE GRAPHICS 3D) E TRASFORMAZIONI GEOMETRICHE
// =========================================================================
// Traduzione dei simboli della stringa finale in primitive grafiche tridimensionali
// mediante manipolazione della matrice Modelview di OpenGL.
// =========================================================================

char* stringaFinale = NULL;

// Parametri ricalibrati per un albero grande e ben visibile
float angoloRotazione = 25.0f; 
float lunghezzaRamo = 0.4f;    // Aumentato per farlo crescere in altezza
float raggioBase = 0.05f;      // Spessore proporzionato

// Telecamera ricalcolata per CENTRARE l'albero a schermo
float camPosX = 0.0f;
float camPosY = -3.5f;     // Bilancia l'altezza totale per metterlo al centro
float camPosZ = -18.0f;    // Arretra a sufficienza per inquadrarlo tutto
float camAngoloX = 15.0f;  
float camAngoloY = 0.0f;

int staTrascinando = 0;
int ultimoMouseX = 0;
int ultimoMouseY = 0;

/**
 * Rendering geometrico del singolo segmento (ramo).
 * Sfrutta le librerie GLU per modellare un tronco conico rastremato verso l'alto.
 */
void disegnaCilindroGLU(float raggio, float altezza) {
    GLUquadricObj* qobj = gluNewQuadric();
    gluQuadricDrawStyle(qobj, GLU_FILL);
    gluQuadricNormals(qobj, GLU_SMOOTH);
    
    // Restringimento conico del ramo
    gluCylinder(qobj, raggio, raggio * 0.8f, altezza, 10, 1);
    gluDeleteQuadric(qobj);
}

/**
 * Automa a Stati Finiti (Parsing ed Esecuzione dell'L-System).
 * Interpreta i caratteri della stringa applicando le relative matrici di rototraslazione.
 */
void interpretaStringa() {
    if (stringaFinale == NULL) return;

    float mock_posX = 0.0f, mock_posY = 0.0f, mock_posZ = 0.0f;
    float mock_rotX = 0.0f, mock_rotY = 0.0f;

    // COLORE VERDE come richiesto
    glColor3f(0.15f, 0.70f, 0.20f); 

    int i = 0;
    while (stringaFinale[i] != '\0') {
        char comando = stringaFinale[i];

        switch (comando) {
            case 'F': 
                disegnaCilindroGLU(raggioBase, lunghezzaRamo);
                glTranslatef(0.0f, 0.0f, lunghezzaRamo);
                break;

            case '+': 
                glRotatef(angoloRotazione, 1.0f, 0.0f, 0.0f); 
                mock_rotX += angoloRotazione;
                break;

            case '-': 
                glRotatef(-angoloRotazione, 1.0f, 0.0f, 0.0f); 
                mock_rotX -= angoloRotazione;
                break;

            case '[': 
                // Ramificazione: push dello stato nello stack di memoria e push della matrice corrente
                push(mock_posX, mock_posY, mock_posZ, mock_rotX, mock_rotY);
                glPushMatrix();
                
                // Rotazione tridimensionale (Angolo di divergenza basato sulla fillotassi)
                glRotatef(137.5f, 0.0f, 0.0f, 1.0f);
                
                // I rami figli diventano più sottili del 30%
                raggioBase *= 0.70f;
                break;

            case ']': 
                // Ritorno dal ramo: pop dello stato e ripristino della matrice di trasformazione precedente
                pop(); 
                glPopMatrix();
                
                // Ripristina il raggio
                raggioBase /= 0.70f; 
                break;

            default:
                break;
        }
        i++;
    }
}

// =========================================================================
// 4. RENDERING LOOP E INTERAZIONE UTENTE (INTERFACCIA INPUT)
// =========================================================================
// Sezione di gestione della pipeline grafica (display), dello spazio di 
// proiezione e delle routine di callback per mouse e tastiera.
// =========================================================================

/**
 * Callback di rendering principale.
 * Pulisce i buffer, imposta la vista della camera (Modelview) ed esegue il disegno.
 */
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Traslazione calcolata per centrare l'oggetto
    glTranslatef(camPosX, camPosY, camPosZ);
    glRotatef(camAngoloX, 1.0f, 0.0f, 0.0f);
    glRotatef(camAngoloY, 0.0f, 1.0f, 0.0f);

    // Orientamento del sistema di riferimento: asse Z orientato verso l'alto
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    // Raggio iniziale del tronco
    raggioBase = 0.15f; 
    interpretaStringa();

    glutSwapBuffers();
}

/**
 * Gestione degli input da tastiera per la navigazione libera nello spazio 3D.
 */
void keyboard(unsigned char key, int x, int y) {
    float velocita = 0.5f;
    switch (key) {
        case 'w': case 'W': camPosZ += velocita; break; // Avanti
        case 's': case 'S': camPosZ -= velocita; break; // Indietro
        case 'a': case 'A': camPosX += velocita; break; // Sinistra
        case 'd': case 'D': camPosX -= velocita; break; // Destra
        
        // NUOVI TASTI PER MOVIMENTO VERTICALE
        case 'q': case 'Q': camPosY -= velocita; break; // Alza visuale (sposta camera giù)
        case 'e': case 'E': camPosY += velocita; break; // Abbassa visuale (sposta camera su)
        
        case 27: exit(0); break; // ESC per uscire
    }
    glutPostRedisplay();
}

/**
 * Gestione del click del mouse per attivare lo stato di trascinamento (Arcball rotazione).
 */
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            staTrascinando = 1;
            ultimoMouseX = x;
            ultimoMouseY = y;
        } else {
            staTrascinando = 0;
        }
    }
}

/**
 * Calcolo del delta di movimento del mouse per la rotazione interattiva della scena.
 */
void motion(int x, int y) {
    if (staTrascinando) {
        float dx = (float)(x - ultimoMouseX);
        float dy = (float)(y - ultimoMouseY);
        camAngoloY += dx * 0.5f; 
        camAngoloX += dy * 0.5f; 
        ultimoMouseX = x;
        ultimoMouseY = y;
        glutPostRedisplay();
    }
}

/**
 * Setup dei parametri di rendering: Abilitazione del Depth Test, Materiali e Illuminazione (Phong).
 */
void inizializzaOpenGL() {
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f); 
    glEnable(GL_DEPTH_TEST);                 

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat light_pos[] = { 10.0f, 15.0f, 10.0f, 1.0f };
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

/**
 * Routine di Reshape per mantenere il corretto Aspect Ratio della finestra di proiezione prospettica.
 */
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, aspect, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// =========================================================================
// 5. ENTRY POINT (MAIN FUNCTION)
// =========================================================================
// Inizializzazione della stringa formale, configurazione del contesto 
// GLUT e avvio del ciclo principale degli eventi.
// =========================================================================

int main(int argc, char** argv) {
    // Generazioni riportate a 4 per mantenere una forma chiara e non troppo caotica
    int generazioni = 4; 
    stringaFinale = generaLSystem("F", generazioni); 
    
    printf("==================================================\n");
    printf(" L-SYSTEM PROCEDURAL GENERATOR - Tesina \n");
    printf(" Candidato: Fabio Bandiera \n");
    printf("==================================================\n");
    printf(" Assioma: F\n");
    printf(" Regola formale: F -> FF+[+F-F] o simili\n");
    printf(" Generazioni calcolate: %d\n", generazioni);
    printf("==================================================\n");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Generazione Procedurale L-Systems - Fabio Bandiera");

    inizializzaOpenGL();
    
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();

    // Rilascio definitivo della memoria heap per prevenire Memory Leak
    free(stringaFinale);
    return 0;
}