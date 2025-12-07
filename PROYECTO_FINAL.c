/*
COMPILACIÓN:
cd "C:\Users\renat\Desktop\AGO25-ENE26\Graficacion"
gcc PROYECTO_FINAL.c -o PROYECTO_FINAL.exe -lfreeglut -lopengl32 -lglu32 -mconsole
PROYECTO_FINAL.exe

*/
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define PI 3.14159265359
#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

// Límites de movimiento de articulaciones
#define MUSLO_MIN 1.67
#define MUSLO_MAX 87.62
#define RODILLA_MIN -78.54
#define RODILLA_MAX 1.67
#define BRAZO_MIN 25.99
#define BRAZO_MAX 169.23
#define ANTEBRAZO_MIN 25.99
#define ANTEBRAZO_MAX 82.17


// NODO para represnetar cada arituclacion
typedef struct Nodo {
    char nombre[50];              
    double tx, ty;                // Posición relativa al padre
    double angulo;                // Rotación actual
    double longitud;              // Tamaño del segmento
    struct Nodo* padre;           // Puntero al nodo padre
    struct Nodo* primerHijo;      // Primer hijo en la jerarquía
    struct Nodo* siguienteHermano;// Siguiente nodo al mismo nivel
} Nodo;

// esta es la estructura de mi personaje , en mi caso es una mujer 
typedef struct Personaje {
    char nombre[50];
    double posX, posY;            // la pos del personaje en el plano
    double escala;                // tamaño del personaje
    float colorVestido[3];        
    float colorVestidoOscuro[3];  
    float colorCuello[3];       
    // Ángulos de cada articulación (en radianes)
    double anguloBrazoDer, anguloAntebrazoDer;
    double anguloBrazoIzq, anguloAntebrazoIzq;
    double anguloMusloDer, anguloPantorrillaDer;
    double anguloMusloIzq, anguloPantorrillaIzq;
    Nodo* torso;                  // Raíz del árbol jerárquico
    
    float tiempoAnimacion;        // Tiempo acumulado
    int tipoAnimacion;            // es q tengo pensado en que tenga varias animaciones a lo largo de la cinematica
    int animando;                 // 1=animando, 0=quieto
} Personaje;


// proporciones inciiales del cuerpo
double centroTorso[2] = {0.0, 0.069};
double hombroDer[2] = {0.12, 0.2};
//vector de dirección y longitud, a donde apunta y que tan largo es 
double vectorBrazoDer[4] = {0.02, -0.16, 0, 1};
double vectorAntebrazoDer[4] = {0.06, -0.14, 0, 1};
double hombroIzq[2] = {-0.12, 0.2};
double vectorBrazoIzq[4] = {-0.02, -0.16, 0, 1};
double vectorAntebrazoIzq[4] = {-0.06, -0.14, 0, 1};
double inicioMusloDer[2] = {0.121, -0.44};
double vectorMusloDer[4] = {0.0, -0.16, 0, 1};
double vectorPantorrillaDer[4] = {0.0, -0.12, 0, 1};
double inicioMusloIzq[2] = {-0.080, -0.44};
double vectorMusloIzq[4] = {0.0, -0.16, 0, 1};
double vectorPantorrillaIzq[4] = {0.0, -0.12, 0, 1};

int ventana_principal;            // ID ventana de animación
int ventana_menu;                 // ID ventana de menú
Personaje* personajes[10];        // Array de personajes 
int numPersonajes = 0;            // Contador actual
int animacion_pausada = 0;        // Estado: 0=play, 1=pause
int opcion_seleccionada = 0;      // Opción del menú seleccionada
int menu_activo = 1;              // Visibilidad del menú

double limitarAngulo(double angulo, double min, double max);
void dibujarCirculo(float x, float y, float radio, int segmentos);
void dibujarLinea(float x1, float y1, float x2, float y2);
void dibujarPoligono(float vertices[][2], int numVertices);
Nodo* crearNodo(char* nombre, double tx, double ty, double longitud, double angulo);
void agregarHijo(Nodo* padre, Nodo* hijo);
void construirArbolPersonaje(Personaje* p);
void liberarArbol(Nodo* nodo);
void dibujarVestidoPersonaje(Personaje* p);
void dibujarCabezaPersonaje(Personaje* p);
void dibujarNodoPersonaje(Nodo* nodo);
void actualizarAngulosPersonaje(Personaje* p);
void animarPersonaje(Personaje* p);
void dibujarPersonaje(Personaje* p);
void dibujarBotonMenu(float x, float y, float ancho, float alto, char *texto, int seleccionado);
void displayMenu(void);
void tecladoMenu(unsigned char key, int x, int y);
void tecladoEspecialMenu(int key, int x, int y);
Personaje* crearPersonaje(char* nombre, double x, double y, double escala, float r, float g, float b, int tipoAnimacion);
void eliminarPersonaje(Personaje* p);
void display();
void redibujo();
void teclado(unsigned char key, int x, int y);
void dibujarFlor(float x, float y, float radio, float r, float g, float b);


// Limita un ángulo al rango permitido
double limitarAngulo(double angulo, double min, double max) {
    double angulo_grados = angulo * RAD_TO_DEG;
    if (angulo_grados < min) angulo_grados = min;
    if (angulo_grados > max) angulo_grados = max;
    return angulo_grados * DEG_TO_RAD;
}

void dibujarCirculo(float x, float y, float radio, int segmentos) {
    glBegin(GL_POLYGON);
    for(int i = 0; i < segmentos; i++) {
        float angulo = (2.0 * PI * i) / segmentos;//Se calcu el ang de cara vertice
        float px = x + radio * cos(angulo);
        float py = y + radio * sin(angulo);
        glVertex2f(px, py);
    }
    glEnd();
}

// Dibuja una línea entre dos puntos
void dibujarLinea(float x1, float y1, float x2, float y2) {
    glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    glEnd();
}

// Dibuja un polígono dado un array de vértices
void dibujarPoligono(float vertices[][2], int numVertices) {
    glBegin(GL_POLYGON);
    for(int i = 0; i < numVertices; i++) {
        glVertex2f(vertices[i][0], vertices[i][1]);
    }
    glEnd();
}

// Crea un nodo nuevo para el árbol jerárquico
Nodo* crearNodo(char* nombre, double tx, double ty, double longitud, double angulo) {
    Nodo* nuevo = (Nodo*)malloc(sizeof(Nodo));
    strcpy(nuevo->nombre, nombre);
    nuevo->tx = tx;
    nuevo->ty = ty;
    nuevo->angulo = angulo;
    nuevo->longitud = longitud;
    nuevo->padre = NULL;
    nuevo->primerHijo = NULL;
    nuevo->siguienteHermano = NULL;
    return nuevo;
}

// Agrega un hijo a un nodo en el árbol
void agregarHijo(Nodo* padre, Nodo* hijo) {
    hijo->padre = padre;
    if (padre->primerHijo == NULL) {
        padre->primerHijo = hijo;
    } else {
        Nodo* hermano = padre->primerHijo;
        while (hermano->siguienteHermano != NULL) {
            hermano = hermano->siguienteHermano;
        }
        hermano->siguienteHermano = hijo;
    }
}

// Libera recursivamente toda la memoria del árbol
void liberarArbol(Nodo* nodo) {
    if (nodo == NULL) return;
    liberarArbol(nodo->primerHijo);
    liberarArbol(nodo->siguienteHermano);
    free(nodo);
}

// Construye el árbol jerárquico completo del personaje
void construirArbolPersonaje(Personaje* p) {
    // Calcular longitudes de cada segmento
    double longBrazoDer = sqrt(vectorBrazoDer[0]*vectorBrazoDer[0] + vectorBrazoDer[1]*vectorBrazoDer[1]);
    double longAntebrazoDer = sqrt(vectorAntebrazoDer[0]*vectorAntebrazoDer[0] + vectorAntebrazoDer[1]*vectorAntebrazoDer[1]);
    double longBrazoIzq = sqrt(vectorBrazoIzq[0]*vectorBrazoIzq[0] + vectorBrazoIzq[1]*vectorBrazoIzq[1]);
    double longAntebrazoIzq = sqrt(vectorAntebrazoIzq[0]*vectorAntebrazoIzq[0] + vectorAntebrazoIzq[1]*vectorAntebrazoIzq[1]);
    double longMusloDer = sqrt(vectorMusloDer[0]*vectorMusloDer[0] + vectorMusloDer[1]*vectorMusloDer[1]);
    double longPantorrillaDer = sqrt(vectorPantorrillaDer[0]*vectorPantorrillaDer[0] + vectorPantorrillaDer[1]*vectorPantorrillaDer[1]);
    double longMusloIzq = sqrt(vectorMusloIzq[0]*vectorMusloIzq[0] + vectorMusloIzq[1]*vectorMusloIzq[1]);
    double longPantorrillaIzq = sqrt(vectorPantorrillaIzq[0]*vectorPantorrillaIzq[0] + vectorPantorrillaIzq[1]*vectorPantorrillaIzq[1]);
    
    // RAÍZ: Torso 
    p->torso = crearNodo("Torso", centroTorso[0], centroTorso[1], 0, 0);
    
    // BRAZO DERECHO: Torso - Brazo - Antebrazo - Mano
    Nodo* brazoDer = crearNodo("Brazo Derecho", hombroDer[0], hombroDer[1], longBrazoDer, p->anguloBrazoDer);
    agregarHijo(p->torso, brazoDer);
    Nodo* antebrazoDer = crearNodo("Antebrazo Derecho", 0, -longBrazoDer, longAntebrazoDer, p->anguloAntebrazoDer);
    agregarHijo(brazoDer, antebrazoDer);
    Nodo* manoDer = crearNodo("Mano Derecha", 0, -longAntebrazoDer, 0, 0);
    agregarHijo(antebrazoDer, manoDer);
    
    // BRAZO IZQUIERDO: Torso - Brazo - Antebrazo - Mano
    Nodo* brazoIzq = crearNodo("Brazo Izquierdo", hombroIzq[0], hombroIzq[1], longBrazoIzq, p->anguloBrazoIzq);
    agregarHijo(p->torso, brazoIzq);
    Nodo* antebrazoIzq = crearNodo("Antebrazo Izquierdo", 0, -longBrazoIzq, longAntebrazoIzq, p->anguloAntebrazoIzq);
    agregarHijo(brazoIzq, antebrazoIzq);
    Nodo* manoIzq = crearNodo("Mano Izquierda", 0, -longAntebrazoIzq, 0, 0);
    agregarHijo(antebrazoIzq, manoIzq);
    
    // PIERNA DERECHA: Torso- Muslo - Pantorrilla - Pie
    Nodo* piernaDer = crearNodo("Pierna Derecha", inicioMusloDer[0], inicioMusloDer[1], longMusloDer, p->anguloMusloDer);
    agregarHijo(p->torso, piernaDer);
    Nodo* pantorrillaDer = crearNodo("Pantorrilla Derecha", 0, -longMusloDer, longPantorrillaDer, p->anguloPantorrillaDer);
    agregarHijo(piernaDer, pantorrillaDer);
    Nodo* pieDer = crearNodo("Pie Derecho", 0, -longPantorrillaDer, 0, 0);
    agregarHijo(pantorrillaDer, pieDer);
    
    // PIERNA IZQUIERDA: Torso -> Muslo -> Pantorrilla -> Pie
    Nodo* piernaIzq = crearNodo("Pierna Izquierda", inicioMusloIzq[0], inicioMusloIzq[1], longMusloIzq, p->anguloMusloIzq);
    agregarHijo(p->torso, piernaIzq);
    Nodo* pantorrillaIzq = crearNodo("Pantorrilla Izquierda", 0, -longMusloIzq, longPantorrillaIzq, p->anguloPantorrillaIzq);
    agregarHijo(piernaIzq, pantorrillaIzq);
    Nodo* pieIzq = crearNodo("Pie Izquierdo", 0, -longPantorrillaIzq, 0, 0);
    agregarHijo(pantorrillaIzq, pieIzq);
}

// Actualiza recursivamente los ángulos en el árbol
void actualizarAngulosEnNodos(Nodo* nodo, Personaje* p) {
    if (nodo == NULL) return;
    
    // Actualizar según el nombre del nodo
    if (strcmp(nodo->nombre, "Brazo Derecho") == 0)
        nodo->angulo = limitarAngulo(p->anguloBrazoDer, BRAZO_MIN, BRAZO_MAX);
    else if (strcmp(nodo->nombre, "Antebrazo Derecho") == 0)
        nodo->angulo = limitarAngulo(p->anguloAntebrazoDer, ANTEBRAZO_MIN, ANTEBRAZO_MAX);
    else if (strcmp(nodo->nombre, "Brazo Izquierdo") == 0)
        nodo->angulo = limitarAngulo(p->anguloBrazoIzq, BRAZO_MIN, BRAZO_MAX);
    else if (strcmp(nodo->nombre, "Antebrazo Izquierdo") == 0)
        nodo->angulo = limitarAngulo(p->anguloAntebrazoIzq, ANTEBRAZO_MIN, ANTEBRAZO_MAX);
    else if (strcmp(nodo->nombre, "Pierna Derecha") == 0)
        nodo->angulo = limitarAngulo(p->anguloMusloDer, MUSLO_MIN, MUSLO_MAX);
    else if (strcmp(nodo->nombre, "Pantorrilla Derecha") == 0)
        nodo->angulo = limitarAngulo(p->anguloPantorrillaDer, RODILLA_MIN, RODILLA_MAX);
    else if (strcmp(nodo->nombre, "Pierna Izquierda") == 0)
        nodo->angulo = limitarAngulo(p->anguloMusloIzq, MUSLO_MIN, MUSLO_MAX);
    else if (strcmp(nodo->nombre, "Pantorrilla Izquierda") == 0)
        nodo->angulo = limitarAngulo(p->anguloPantorrillaIzq, RODILLA_MIN, RODILLA_MAX);
    
    // Continuar con hijos y hermanos
    actualizarAngulosEnNodos(nodo->primerHijo, p);
    actualizarAngulosEnNodos(nodo->siguienteHermano, p);
}

void actualizarAngulosPersonaje(Personaje* p) {
    actualizarAngulosEnNodos(p->torso, p);
}

// Dibuja el vestido del personaje
void dibujarVestidoPersonaje(Personaje* p) {
    glColor3f(p->colorVestido[0], p->colorVestido[1], p->colorVestido[2]);
    
    float torsoSuperior[][2] = {
        {-0.12 + centroTorso[0], 0.2 + centroTorso[1]},
        {0.12 + centroTorso[0], 0.2 + centroTorso[1]},
        {0.0808656432054 + centroTorso[0], -0.0498050953935 + centroTorso[1]},
        {-0.0791019006464 + centroTorso[0], -0.0504133370051 + centroTorso[1]}
    };
    dibujarPoligono(torsoSuperior, 4);
    
    float falda[][2] = {
        {-0.0791019006464 + centroTorso[0], -0.0504133370051 + centroTorso[1]},
        {0.0808656432054 + centroTorso[0], -0.0498050953935 + centroTorso[1]},
        {0.2472774325569 + centroTorso[0], -0.4431154961989 + centroTorso[1]},
        {-0.2458433796982 + centroTorso[0], -0.4450801209489 + centroTorso[1]}
    };
    dibujarPoligono(falda, 4);
    
    glColor3f(p->colorVestidoOscuro[0], p->colorVestidoOscuro[1], p->colorVestidoOscuro[2]);
    glLineWidth(2.0);
    dibujarLinea(-0.06 + centroTorso[0], 0.15 + centroTorso[1], -0.12 + centroTorso[0], -0.35 + centroTorso[1]);
    dibujarLinea(0.0 + centroTorso[0], 0.15 + centroTorso[1], 0.0 + centroTorso[0], -0.40 + centroTorso[1]);
    dibujarLinea(0.06 + centroTorso[0], 0.15 + centroTorso[1], 0.12 + centroTorso[0], -0.35 + centroTorso[1]);
    
    glColor3f(p->colorCuello[0], p->colorCuello[1], p->colorCuello[2]);
    float cuello[][2] = {
        {-0.02 + centroTorso[0], 0.22 + centroTorso[1]},
        {0.02 + centroTorso[0], 0.22 + centroTorso[1]},
        {0.02 + centroTorso[0], 0.2 + centroTorso[1]},
        {-0.02 + centroTorso[0], 0.2 + centroTorso[1]}
    };
    dibujarPoligono(cuello, 4);
}

void dibujarFlor(float x, float y, float radio, float r, float g, float b) {
    glColor3f(r * 0.7, g * 0.5, 0.0);
    dibujarCirculo(x, y, radio * 0.4, 15);
    
    glColor3f(r, g, b);
    for(int i = 0; i < 8; i++) {
        float angulo = (2.0 * PI * i) / 8.0;
        float px = x + radio * 0.7 * cos(angulo);
        float py = y + radio * 0.7 * sin(angulo);
        dibujarCirculo(px, py, radio * 0.5, 12);
    }
}


void dibujarCabezaPersonaje(Personaje* p) {
    glColor3f(0.95, 0.87, 0.78);
    float cuelloPersona[][2] = {
        {-0.025 + centroTorso[0], 0.22 + centroTorso[1]},
        {0.025 + centroTorso[0], 0.22 + centroTorso[1]},
        {0.025 + centroTorso[0], 0.26 + centroTorso[1]},
        {-0.025 + centroTorso[0], 0.26 + centroTorso[1]}
    };
    dibujarPoligono(cuelloPersona, 4);
    
    dibujarCirculo(centroTorso[0], 0.36 + centroTorso[1], 0.08, 30);
    
    glColor3f(0.15, 0.10, 0.08);
    dibujarCirculo(centroTorso[0], 0.37 + centroTorso[1], 0.095, 30);
    
    float mechonIzq[][2] = {
        {-0.08 + centroTorso[0], 0.36 + centroTorso[1]},
        {-0.10 + centroTorso[0], 0.30 + centroTorso[1]},
        {-0.11 + centroTorso[0], 0.20 + centroTorso[1]},
        {-0.10 + centroTorso[0], 0.19 + centroTorso[1]},
        {-0.08 + centroTorso[0], 0.28 + centroTorso[1]},
        {-0.075 + centroTorso[0], 0.34 + centroTorso[1]}
    };
    dibujarPoligono(mechonIzq, 6);
    
    float mechonDer[][2] = {
        {0.08 + centroTorso[0], 0.36 + centroTorso[1]},
        {0.10 + centroTorso[0], 0.30 + centroTorso[1]},
        {0.11 + centroTorso[0], 0.20 + centroTorso[1]},
        {0.10 + centroTorso[0], 0.19 + centroTorso[1]},
        {0.08 + centroTorso[0], 0.28 + centroTorso[1]},
        {0.075 + centroTorso[0], 0.34 + centroTorso[1]}
    };
    dibujarPoligono(mechonDer, 6);
    
   
    //dibujarFlor(-0.075 + centroTorso[0], 0.40 + centroTorso[1], 1.0, 0.8, 0.9);
}

// Dibuja recursivamente un nodo y sus hijos (recorrido del árbol)
void dibujarNodoPersonaje(Nodo* nodo) {
    if (nodo == NULL) return;
    
    glPushMatrix();  // Guardar matriz de transformación actual (PILA)
        // Aplicar transformaciones locales del nodo
        glTranslatef(nodo->tx, nodo->ty, 0);
        glRotatef(nodo->angulo * 180.0 / PI, 0, 0, 1);
        
        // Dibujar según el tipo de nodo
        if (strcmp(nodo->nombre, "Brazo Derecho") == 0 || strcmp(nodo->nombre, "Brazo Izquierdo") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            glLineWidth(12.0);
            dibujarLinea(0, 0, 0, -nodo->longitud);
        }
        else if (strcmp(nodo->nombre, "Antebrazo Derecho") == 0 || strcmp(nodo->nombre, "Antebrazo Izquierdo") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            glLineWidth(10.0);
            dibujarLinea(0, 0, 0, -nodo->longitud);
        }
        else if (strcmp(nodo->nombre, "Mano Derecha") == 0 || strcmp(nodo->nombre, "Mano Izquierda") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            dibujarCirculo(0, 0, 0.025, 20);
        }
        else if (strcmp(nodo->nombre, "Pierna Derecha") == 0 || strcmp(nodo->nombre, "Pierna Izquierda") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            glLineWidth(15.0);
            dibujarLinea(0, 0, 0, -nodo->longitud);
        }
        else if (strcmp(nodo->nombre, "Pantorrilla Derecha") == 0 || strcmp(nodo->nombre, "Pantorrilla Izquierda") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            glLineWidth(12.0);
            dibujarLinea(0, 0, 0, -nodo->longitud);
        }
        else if (strcmp(nodo->nombre, "Pie Derecho") == 0 || strcmp(nodo->nombre, "Pie Izquierdo") == 0) {
            glColor3f(0.95, 0.87, 0.78);
            dibujarCirculo(0, 0, 0.030, 20);
        }
        
        // Dibujar hijos (recursión en profundidad)
        dibujarNodoPersonaje(nodo->primerHijo);
    glPopMatrix();  // Restaurar matriz anterior (PILA)
    
    // Dibujar hermanos (recursión en anchura)
    dibujarNodoPersonaje(nodo->siguienteHermano);
}

// ANIMACIONES: si me da timepo meto mas, si no pues no ajsjajaj
void animarPersonaje(Personaje* p) {
    if (!p->animando) return;
    
    double rango_brazo = (BRAZO_MAX - BRAZO_MIN) * DEG_TO_RAD;
    double centro_brazo = (BRAZO_MAX + BRAZO_MIN) * 0.5 * DEG_TO_RAD;
    double rango_antebrazo = (ANTEBRAZO_MAX - ANTEBRAZO_MIN) * DEG_TO_RAD;
    double centro_antebrazo = (ANTEBRAZO_MAX + ANTEBRAZO_MIN) * 0.5 * DEG_TO_RAD;
    
    switch(p->tipoAnimacion) {
        case 0: // Estático
            break;
            
        case 1: // Saludo con ambos brazos
            p->anguloBrazoDer = centro_brazo + sin(p->tiempoAnimacion * 2.0) * (rango_brazo * 0.4);
            p->anguloAntebrazoDer = centro_antebrazo + sin(p->tiempoAnimacion * 3.0) * (rango_antebrazo * 0.35);
            p->anguloBrazoIzq = centro_brazo + sin(p->tiempoAnimacion * 2.0 + PI) * (rango_brazo * 0.35);
            p->anguloAntebrazoIzq = centro_antebrazo + sin(p->tiempoAnimacion * 2.5 + PI/2) * (rango_antebrazo * 0.3);
            break;
            
        case 2: // Brazos arriba (celebración)
            p->anguloBrazoDer = BRAZO_MAX * DEG_TO_RAD + sin(p->tiempoAnimacion * 3.0) * 0.2;
            p->anguloAntebrazoDer = ANTEBRAZO_MAX * DEG_TO_RAD;
            p->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD + sin(p->tiempoAnimacion * 3.0 + PI) * 0.2;
            p->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD;
            break;
            
        case 3: // Movimiento suave
            p->anguloBrazoDer = centro_brazo + sin(p->tiempoAnimacion * 1.0) * (rango_brazo * 0.2);
            p->anguloAntebrazoDer = centro_antebrazo + sin(p->tiempoAnimacion * 1.5) * (rango_antebrazo * 0.15);
            p->anguloBrazoIzq = centro_brazo + sin(p->tiempoAnimacion * 1.0 + PI/3) * (rango_brazo * 0.2);
            p->anguloAntebrazoIzq = centro_antebrazo + sin(p->tiempoAnimacion * 1.5 + PI/3) * (rango_antebrazo * 0.15);
            break;
    }
    
    actualizarAngulosPersonaje(p);
}

// Dibuja el personaje completo con todas sus transformaciones
void dibujarPersonaje(Personaje* p) {
    glPushMatrix();  // Guardar estado (PILA)
        glTranslatef(p->posX, p->posY, 0);  // Mover a posición en el mundo
        glScalef(p->escala, p->escala, 1.0); // Aplicar escala
        
        dibujarVestidoPersonaje(p);
        dibujarNodoPersonaje(p->torso);  // Dibujar árbol jerárquico
        dibujarCabezaPersonaje(p);
    glPopMatrix();  // Restaurar estado (PILA)
}

// Crea un nuevo personaje con sus características
Personaje* crearPersonaje(char* nombre, double x, double y, double escala, float r, float g, float b, int tipoAnimacion) {
    Personaje* p = (Personaje*)malloc(sizeof(Personaje));
    strcpy(p->nombre, nombre);
    p->posX = x;
    p->posY = y;
    p->escala = escala;
    
    // Configurar colores
    p->colorVestido[0] = r;
    p->colorVestido[1] = g;
    p->colorVestido[2] = b;
    p->colorVestidoOscuro[0] = r * 0.7;
    p->colorVestidoOscuro[1] = g * 0.7;
    p->colorVestidoOscuro[2] = b * 0.7;
    p->colorCuello[0] = r * 1.2;
    p->colorCuello[1] = g * 1.2;
    p->colorCuello[2] = b * 1.2;
    
    // Ángulos iniciales
    p->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    p->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    p->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    p->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
    p->anguloMusloDer = MUSLO_MIN * DEG_TO_RAD;
    p->anguloPantorrillaDer = RODILLA_MAX * DEG_TO_RAD;
    p->anguloMusloIzq = MUSLO_MIN * DEG_TO_RAD;
    p->anguloPantorrillaIzq = RODILLA_MAX * DEG_TO_RAD;
    
    // Configurar animación
    p->tiempoAnimacion = 0.0;
    p->tipoAnimacion = tipoAnimacion;
    p->animando = (tipoAnimacion != 0); 
    
    // Construir árbol jerárquico
    construirArbolPersonaje(p);
    
    return p;
}

// Libera la memoria de un personaje
void eliminarPersonaje(Personaje* p) {
    if (p == NULL) return;
    liberarArbol(p->torso);
    free(p);
}

// Dibuja la escena de animación
void display() {
    glClear(GL_COLOR_BUFFER_BIT);  // Limpiar pantalla
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Dibujar todos los personajes activos
    for(int i = 0; i < numPersonajes; i++) {
        dibujarPersonaje(personajes[i]);
    }
    
    // Indicador de pausa
    if(animacion_pausada) {
        glColor3f(1.0, 1.0, 0.0);
        glRasterPos2f(-0.15, 0.95);
        char pausa[] = "|| PAUSADO ||";
        for(int i = 0; pausa[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, pausa[i]);
        }
    }
    
    glutSwapBuffers();
}



//DECORACIONES, AUN NO SE USAAAAAN 

void dibujarCalavera(float x, float y, float tamano) {
    glColor3f(0.98, 0.98, 1.0);
    dibujarCirculo(x, y + tamano * 0.15, tamano * 0.25, 25);
    
    glBegin(GL_POLYGON);
        glVertex2f(x - tamano * 0.2, y + tamano * 0.05);
        glVertex2f(x + tamano * 0.2, y + tamano * 0.05);
        glVertex2f(x + tamano * 0.15, y - tamano * 0.1);
        glVertex2f(x - tamano * 0.15, y - tamano * 0.1);
    glEnd();
    
    glColor3f(0.0, 0.0, 0.0);
    dibujarCirculo(x - tamano * 0.12, y + tamano * 0.2, tamano * 0.08, 20);
    dibujarCirculo(x + tamano * 0.12, y + tamano * 0.2, tamano * 0.08, 20);
    
    glBegin(GL_TRIANGLES);
        glVertex2f(x, y + tamano * 0.05);
        glVertex2f(x - tamano * 0.04, y + tamano * 0.15);
        glVertex2f(x + tamano * 0.04, y + tamano * 0.15);
    glEnd();
    
    glColor3f(0.1, 0.1, 0.1);
    glLineWidth(2.0);
    for(int i = 0; i < 6; i++) {
        float dx = x - tamano * 0.12 + (i * tamano * 0.05);
        glBegin(GL_LINES);
            glVertex2f(dx, y);
            glVertex2f(dx, y - tamano * 0.08);
        glEnd();
    }
    
    glColor3f(1.0, 0.2, 0.5);
    dibujarCirculo(x, y + tamano * 0.35, tamano * 0.08, 15);
    
    glColor3f(0.3, 0.8, 1.0);
    dibujarCirculo(x - tamano * 0.15, y + tamano * 0.3, tamano * 0.05, 12);
    dibujarCirculo(x + tamano * 0.15, y + tamano * 0.3, tamano * 0.05, 12);
    
    glColor3f(1.0, 0.8, 0.2);
    dibujarCirculo(x - tamano * 0.12, y + tamano * 0.2, tamano * 0.03, 10);
    dibujarCirculo(x + tamano * 0.12, y + tamano * 0.2, tamano * 0.03, 10);
}


//esta funcion tal vez la use , pero chance ocupo texturas de fondo 

void dibujarEstrellas(void) {
    glColor3f(1.0, 1.0, 0.95);
    
    float estrellas[][2] = {
        {-0.85, 0.75}, {-0.6, 0.82}, {-0.35, 0.68}, {-0.7, 0.55},
        {0.15, 0.78}, {0.45, 0.7}, {0.72, 0.85}, {0.35, 0.88},
        {-0.92, 0.58}, {0.08, 0.92}, {0.82, 0.58}, {-0.25, 0.92},
        {-0.45, 0.48}, {0.6, 0.5}, {0.0, 0.65}, {-0.15, 0.8}
    };
    
    for(int i = 0; i < 16; i++) {
        float x = estrellas[i][0];
        float y = estrellas[i][1];
        float tam = 0.012 + (i % 3) * 0.006;
        
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x, y);
            for(int j = 0; j <= 8; j++) {
                float angulo = (2.0 * PI * j) / 8.0;
                float radio = (j % 2 == 0) ? tam : tam * 0.4;
                glVertex2f(x + radio * cos(angulo), y + radio * sin(angulo));
            }
        glEnd();
    }
}








//MI PERSONAJE PRINCIPAL Q AUN NO ENTRA EN ESCENA
void dibujarFantasma(float x, float y, float escala) {
    glColor3f(0.95, 0.95, 1.0);
    dibujarCirculo(x, y + 0.05 * escala, 0.08 * escala, 30);
    
    glBegin(GL_QUADS);
        glVertex2f(x - 0.08 * escala, y + 0.05 * escala);
        glVertex2f(x + 0.08 * escala, y + 0.05 * escala);
        glVertex2f(x + 0.08 * escala, y - 0.08 * escala);
        glVertex2f(x - 0.08 * escala, y - 0.08 * escala);
    glEnd();
    
    float base_y = y - 0.08 * escala;
    for(int i = 0; i < 3; i++) {
        float pos_x = x - 0.05 * escala + (i * 0.05 * escala);
        glBegin(GL_POLYGON);
        for(int j = 0; j <= 10; j++) {
            float angulo = PI + (PI * j / 10.0);
            float px = pos_x + 0.025 * escala * cos(angulo);
            float py = base_y + 0.025 * escala * sin(angulo);
            glVertex2f(px, py);
        }
        glEnd();
    }
    
    glColor3f(0.1, 0.1, 0.1);
    dibujarCirculo(x - 0.03 * escala, y + 0.06 * escala, 0.015 * escala, 20);
    dibujarCirculo(x + 0.03 * escala, y + 0.06 * escala, 0.015 * escala, 20);
    
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    for(int i = 0; i <= 10; i++) {
        float t = i / 10.0;
        float boca_x = x + (t - 0.5) * 0.04 * escala;
        float boca_y = y + 0.02 * escala - (t * (1 - t)) * 0.02 * escala;
        glVertex2f(boca_x, boca_y);
    }
    glEnd();
}

//OUFIT PARA EL FANTASMA
void dibujarGorroNavidad(float x, float y, float escala) {
    // Parte principal roja del gorro
    glColor3f(0.9, 0.15, 0.15);
    glBegin(GL_TRIANGLES);
        glVertex2f(x, y + 0.15 * escala);
        glVertex2f(x - 0.08 * escala, y);
        glVertex2f(x + 0.08 * escala, y);
    glEnd();
    
    // Borde blanco inferior
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.08 * escala, y);
        glVertex2f(x + 0.08 * escala, y);
        glVertex2f(x + 0.08 * escala, y + 0.02 * escala);
        glVertex2f(x - 0.08 * escala, y + 0.02 * escala);
    glEnd();
    
    // Pompón blanco en la punta
    dibujarCirculo(x, y + 0.15 * escala, 0.025 * escala, 15);
}










// se redibuja contienuamente 
void redibujo() {
    Sleep(30); 
    
    // Solo actualizar si no está pausado
    if(!animacion_pausada) {
        for(int i = 0; i < numPersonajes; i++) {
            if(personajes[i]->animando) {
                personajes[i]->tiempoAnimacion += 0.05;
                animarPersonaje(personajes[i]);
            }
        }
    }
    
    glutPostRedisplay();  // Solicitar redibujo
}

// Manejo de teclado en ventana principal
void teclado(unsigned char key, int x, int y) {
    if(key == 27) { 
        for(int i = 0; i < numPersonajes; i++) {
            eliminarPersonaje(personajes[i]);
        }
        exit(0);
    }
    else if(key == 32) {  
        animacion_pausada = !animacion_pausada;
        printf("Animacion %s\n", animacion_pausada ? "PAUSADA" : "REPRODUCIENDO");
        glutSetWindow(ventana_menu);
        glutPostRedisplay();
        glutSetWindow(ventana_principal);
    }
    else if(key == 'r' || key == 'R') {  
        for(int i = 0; i < numPersonajes; i++) {
            personajes[i]->tiempoAnimacion = 0.0;
        }
        animacion_pausada = 0;
        printf("Animacion REINICIADA\n");
        
        glutSetWindow(ventana_menu);
        glutPostRedisplay();
        glutSetWindow(ventana_principal);
    }
    else if(key == 'm' || key == 'M') {  
        menu_activo = !menu_activo;
        if(menu_activo) {
            glutSetWindow(ventana_menu);
            glutShowWindow();
        } else {
            glutSetWindow(ventana_menu);
            glutHideWindow();
        }
    }
    
    glutPostRedisplay();
}


// Dibuja un botón con texto
void dibujarBotonMenu(float x, float y, float ancho, float alto, char *texto, int seleccionado) {
    // Fondo del botón
    if(seleccionado) {
        glColor3f(0.3, 0.5, 0.8);  
    } else {
        glColor3f(0.2, 0.2, 0.25); 
    }
    
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + ancho, y);
        glVertex2f(x + ancho, y + alto);
        glVertex2f(x, y + alto);
    glEnd();
    
    // Borde
    glColor3f(0.5, 0.5, 0.5);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + ancho, y);
        glVertex2f(x + ancho, y + alto);
        glVertex2f(x, y + alto);
    glEnd();
    
    // Texto
    if(seleccionado) {
        glColor3f(1.0, 1.0, 1.0);  // Blanco si seleccionado
    } else {
        glColor3f(0.8, 0.8, 0.8);  // Gris claro normal
    }
    
    glRasterPos2f(x + 0.05, y + alto * 0.6);
    for(int i = 0; texto[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, texto[i]);
    }
}

// Dibuja la interfaz del menú
void displayMenu(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Fondo degradado
    glBegin(GL_QUADS);
        glColor3f(0.1, 0.1, 0.15);
        glVertex2f(-1.0, -1.0);
        glVertex2f(1.0, -1.0);
        glColor3f(0.05, 0.05, 0.1);
        glVertex2f(1.0, 1.0);
        glVertex2f(-1.0, 1.0);
    glEnd();
    
    // Título
    glColor3f(1.0, 0.6, 0.0);
    glRasterPos2f(-0.65, 0.85);
    char titulo[] = "CONTROL DE ANIMACION";
    for(int i = 0; titulo[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, titulo[i]);
    }
    
    // Botones (estructura de menú)
    float y_base = 0.50;
    float espaciado = 0.22;
    
    dibujarBotonMenu(-0.85, y_base, 1.70, 0.18, animacion_pausada ? "REANUDAR" : "REPRODUCIR", opcion_seleccionada == 0);
    dibujarBotonMenu(-0.85, y_base - espaciado, 1.70, 0.18, "PAUSAR", opcion_seleccionada == 1);
    dibujarBotonMenu(-0.85, y_base - espaciado * 2, 1.70, 0.18, "REINICIAR", opcion_seleccionada == 2);
    dibujarBotonMenu(-0.85, y_base - espaciado * 3, 1.70, 0.18, "SALIR", opcion_seleccionada == 3);
    
    // Instrucciones
    glColor3f(0.6, 0.6, 0.6);
    glRasterPos2f(-0.80, -0.70);
    char inst1[] = "Flechas: Navegar";
    for(int i = 0; inst1[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, inst1[i]);
    }
    
    glRasterPos2f(-0.80, -0.77);
    char inst2[] = "ENTER: Seleccionar";
    for(int i = 0; inst2[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, inst2[i]);
    }
    
    glRasterPos2f(-0.80, -0.84);
    char inst3[] = "Click: Seleccion directa";
    for(int i = 0; inst3[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, inst3[i]);
    }
    
    // Estado actual
    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2f(-0.70, -0.55);
    char estado[50];
    if(animacion_pausada) {
        sprintf(estado, "Estado: PAUSADO");
    } else {
        sprintf(estado, "Estado: REPRODUCIENDO");
    }
    for(int i = 0; estado[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, estado[i]);
    }
    
    glutSwapBuffers();
}

void tecladoMenu(unsigned char key, int x, int y) {
    if(key == 13) {  
        switch(opcion_seleccionada) {
            case 0:  
                animacion_pausada = 0;
                printf("Animacion REPRODUCIENDO\n");
                break;
            case 1:  
                animacion_pausada = 1;
                printf("Animacion PAUSADA\n");
                break;
            case 2:  
                for(int i = 0; i < numPersonajes; i++) {
                    personajes[i]->tiempoAnimacion = 0.0;
                }
                animacion_pausada = 0;
                printf("Animacion REINICIADA\n");
                break;
            case 3:  
                printf("Saliendo del programa...\n");
                for(int i = 0; i < numPersonajes; i++) {
                    eliminarPersonaje(personajes[i]);
                }
                exit(0);
                break;
        }
        glutSetWindow(ventana_menu);
        glutPostRedisplay();
        glutSetWindow(ventana_principal);
        glutPostRedisplay();
    }
    else if(key == 'm' || key == 'M') {  //ocultar menú
        menu_activo = !menu_activo;
        if(menu_activo) {
            glutSetWindow(ventana_menu);
            glutShowWindow();
        } else {
            glutSetWindow(ventana_menu);
            glutHideWindow();
        }
    }
}

//menajo de las flechas 
void tecladoEspecialMenu(int key, int x, int y) {
    if(key == GLUT_KEY_UP) {  // Flecha arriba
        opcion_seleccionada--;
        if(opcion_seleccionada < 0) opcion_seleccionada = 3;
    }
    else if(key == GLUT_KEY_DOWN) {  // Flecha abajo
        opcion_seleccionada++;
        if(opcion_seleccionada > 3) opcion_seleccionada = 0;
    }
    glutSetWindow(ventana_menu);
    glutPostRedisplay();
    glutSetWindow(ventana_principal);
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    
    glutInitWindowSize(1200, 900);
    glutInitWindowPosition(100, 50);
    ventana_principal = glutCreateWindow("Personajes chavas");
    
    glClearColor(0.85, 0.9, 0.95, 1.0); 
    
    //ejemplos de los personajes
    personajes[numPersonajes++] = crearPersonaje("Rubi", 0.0, 0.0, 1.0, 0.55, 0.25, 0.85, 1);
    personajes[numPersonajes++] = crearPersonaje("Roxana", -0.6, 0.0, 0.8, 0.9, 0.2, 0.3, 2);
    personajes[numPersonajes++] = crearPersonaje("Rebeca", 0.6, 0.0, 0.8, 0.2, 0.4, 0.9, 3);
    personajes[numPersonajes++] = crearPersonaje("Renata", -0.4, 0.3, 0.5, 0.3, 0.8, 0.4, 0);
    
    // Registrar callbacks
    glutDisplayFunc(display);
    glutIdleFunc(redibujo);
    glutKeyboardFunc(teclado);
    
    // ventana de menu 
    glutInitWindowSize(350, 700);
    glutInitWindowPosition(1160, 100);
    ventana_menu = glutCreateWindow("Menu ");
    glClearColor(0.1, 0.1, 0.15, 1.0);
    glutDisplayFunc(displayMenu);
    glutKeyboardFunc(tecladoMenu);
    glutSpecialFunc(tecladoEspecialMenu);
    
    printf("\nCONTROLES:\n");
    printf("  ESPACIO - Pausar/Reanudar\n");
    printf("  R - Reiniciar animacion\n");
    printf("  M - Mostrar/Ocultar menu\n");
    printf("  ESC - Salir\n");
    
    glutMainLoop();
    return 0;
}