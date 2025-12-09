/*

glScalef(escala_ancho, escala_alto, 1.0);

*/
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
#endif

#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif

#define PI 3.14159265359
#define RAD_TO_DEG (180.0/PI)
#define DEG_TO_RAD (PI/180.0)

// Límites de movimiento de articulaciones son angulos TODOS RADIANES 
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
    double tx, ty;                // Posición relativa al padre(que tan alejado estal del padre este nodo)
    double angulo;                // Rotación actual, define cuánto está doblada la articulación
    double longitud;              // Tamaño del segmento, del hueso jaajsj
    struct Nodo* padre;           // Puntero al nodo padre
    struct Nodo* primerHijo;      // Primer hijo en la jerarquía
    struct Nodo* siguienteHermano;// Siguiente nodo al mismo nivel
} Nodo;

// esta es la estructura de mi personaje , en mi caso es una mujer, mal hecha, pero cuenta :D
typedef struct Personaje {
    char nombre[50];
    double posX, posY;            // la pos del personaje en el plano
    double escala;                // tamaño del personaje
    float colorVestido[3];        
    float colorVestidoOscuro[3];  
    float colorCuello[3];       
    // Ángulos de cada articulación (en radianes)
    //deifiniendoe la pose inicial, se copian alos nodos correspondiente del arbol
    double anguloBrazoDer, anguloAntebrazoDer;
    double anguloBrazoIzq, anguloAntebrazoIzq;
    double anguloMusloDer, anguloPantorrillaDer;
    double anguloMusloIzq, anguloPantorrillaIzq;
    Nodo* torso;                  // Raíz del árbol jerárquico
    
    float tiempoAnimacion;        // Tiempo acumulado
    int tipoAnimacion;            // es q tengo pensado en que tenga varias animaciones a lo largo de la cinematica
    int animando;                 // 1=animando, 0=quieto, banderita
} Personaje;

//AQUI SE HACE LISTA ENLAZADA
//ESTE ES EL FRAME
typedef struct Keyframe {
    //el moment en seg donde desbe ocurrir este frame 
    float tiempo;
    //almacena los angulos de los brazos en este frame 
    // BRAZOS
    double anguloBrazoDer;
    double anguloAntebrazoDer;
    double anguloBrazoIzq;
    double anguloAntebrazoIzq;
    // PIERNAS
    double anguloMusloDer;
    double anguloPantorrillaDer;
    double anguloMusloIzq;
    double anguloPantorrillaIzq;
      
    // pos del personaje completo en este frame
    float posX;  
    float posY;
    
    struct Keyframe* siguiente;     //puntero al siguiente frame en la lsita enlazada
} Keyframe;

//ESTE ES EL CONJUNTO DE LOS FRAME
//lo veo como el libro de animaciones
typedef struct AnimacionLista{
    Keyframe* primerKeyframe;       //puntero al primer frame de la animacion
    Keyframe* keyframeActual;
    int numKeyframes;               //cuantos fotogramas tiene la animacion 
    float tiempoAnimacion;          //total de duracion de la animacion
} AnimacionLista;

//esta es una strcu para la escenas que va a ir en la cola 
typedef struct Escena {
    int numero;                    //num de la escena
    float duracionInicio;          //segen que inicia
    float duracionFin;             //seg en que termina
    char nombre[50];               
    void (*renderizar)(void);      // Puntero a función de dibujo
    struct Escena* siguiente;      // Siguiente escena en la cola
} Escena;

typedef struct ColaEscenas{
    Escena* frente;                // Primera escena ES LA QUE SE REPRODUCE 
    Escena* final;                 // Última escena agregada
    int totalEscenas;              // Cantidad de escenas
    float tiempoTotal;             // tiempo total de todas las escenas
} ColaEscenas;

//esta si es de chat pq ni idea como se hacia jajsajsja
typedef struct Textura{
    // Es un array unidimensional que contiene los valores de color de TODOS los píxeles
    unsigned char* data;
    int width;      //ancho de la textura en pix
    int height;     //alto de la textura en pix
} Textura;


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
Personaje* personajes[10];        //array de personajes (puse mas de 4 pq quise se optimista y hacer mas pero , parele de contar)
int numPersonajes = 0;            // Contador actual
int animacion_pausada = 1;        // Estado: 0=play, 1=pause
int opcion_seleccionada = 0;      // Opción del menú seleccionada
int menu_activo = 1;              // Visibilidad del menú

AnimacionLista animaciones[10];  //una animación por cada personaje(una vez mas siendo optimista y solo llegue max 4 creo ajasjsjajs)
int animaciones_inicializadas = 0;

// variables de la cola
ColaEscenas colaEscenas = {NULL, NULL, 0, 0.0};//instancia de la cola
int escena_actual = 0;
float alpha_texto = 0.0;        //es para la transparencia del texto   

float fantasma_oscilacion = 0.0;//esta se usa para que el fantasma como que flote
float fantasma_x = -0.8; //posicion horizontal 
float fantasma_y = 0.1;     //posicion vertical
float fantasma_acercamiento = -1.2;// Valores negativos = está alejado, valores positivos = está cerca
float fantasma_escala_inicio = 0.3;  //tam de como ainicia su aparicion, es a escala esta a 30%
float tiempo_total = 0.0;
float fantasma_alejamiento = 2.5;//este es para el alejamiento de la escena final 

int texturas_cargadas = 0;
GLuint texturaInicioMictlan;  //nombrede la textura
Textura texInicioMictlan;//estructura que almacena los datos de la textura (ancho, alto, píxeles)
GLuint texturaFondoSala;
Textura texFondoSala;
GLuint texturaAltar;
Textura texAltar;
GLuint texturaFondoCuarto;
Textura texFondoCuarto;
GLuint texturaFondoGraduacion;
Textura texFondoGraduacion;
GLuint texturaFondoOficina;
Textura texFondoOficina;
GLuint texturacaminoMictlan;
Textura texcaminoMictlan;
GLuint texturaFondoNoche;
Textura texFondoNoche;
GLuint texturaFondoFantasma;
Textura texFondoFantasma;
GLuint texturaFondoCenaNavidad;
Textura texFondoCenaNavidad;
GLuint texturaFondoArbol;
Textura texFondoArbol;
GLuint texturaFondoEsc4;
Textura texFondoEsc4;
GLuint texturafondoEsc7;
Textura texfondoEsc7;

double limitarAngulo(double angulo, double min, double max);
void dibujarCirculo(float x, float y, float radio, int segmentos);
void dibujarLinea(float x1, float y1, float x2, float y2);
void dibujarPoligono(float vertices[][2], int numVertices);
Nodo* crearNodo(char* nombre, double tx, double ty, double longitud, double angulo);
void agregarHijo(Nodo* padre, Nodo* hijo);
void construirArbolPersonaje(Personaje* p);
void liberarArbol(Nodo* nodo);
void actualizarAngulosEnNodos(Nodo* nodo, Personaje* p);
void actualizarAngulosPersonaje(Personaje* p);
void dibujarVestidoPersonaje(Personaje* p);
void dibujarCabezaPersonaje(Personaje* p);
void dibujarNodoPersonaje(Nodo* nodo);
void dibujarPersonaje(Personaje* p);
Personaje* crearPersonaje(char* nombre, double x, double y, double escala, float r, float g, float b, int tipoAnimacion);
void eliminarPersonaje(Personaje* p);
void display();
void dibujarTexto(char *texto, float x, float y);
void dibujarRectangulo(float x, float y, float ancho, float alto);
void dibujarFantasma(float x, float y, float escala);
void dibujarGorroNavidad(float x, float y, float escala);
void redibujo();
void teclado(unsigned char key, int x, int y);
void dibujarBotonMenu(float x, float y, float ancho, float alto, char *texto, int seleccionado);
void displayMenu(void);
void tecladoMenu(unsigned char key, int x, int y);
void tecladoEspecialMenu(int key, int x, int y);
Keyframe* crearKeyframe(float tiempo, Personaje* p);
void agregarKeyframe(AnimacionLista* anim, Keyframe* kf);
void liberarAnimacion(AnimacionLista* anim);
void interpolarKeyframes(Keyframe* k1, Keyframe* k2, float t, Personaje* p);
Keyframe* buscarKeyframeActual(AnimacionLista* anim);
void reproducirAnimacion(int indicePersonaje);
void inicializarAnimaciones();
void encolarEscena(int num, float inicio, float fin, char* nombre, void (*func)(void));
void liberarCola();
void renderizarEscenaActual();
void escena1_presentacion();
void escena2_altar();
void escena3_dialogo();
void escena4_despedida();
void escena5_fantasma_decision(void);
void escena6_transicion_tiempo(void);
void escena7_mujer_verde_cuarto(void);
void escena8_mujer_rosa_oficina(void);
void escena9_mujer_azul_graduacion(void);
void escena10_mujer_morado_computadora(void);
void escena11_transicion_navidad(void);
void escena12_navidad_recuerdos(void);
void escena13_cena_navidad(void);
void escena14_fantasma_despedida(void);
void escena15_despedida_hijas(void);
void escena16_camino_mictlan(void);
void inicializarEscenas();
unsigned char* cargarBMP(const char* filename, int* width, int* height);
void inicializarTexturas();
void dibujarRectanguloTexturizado(float x, float y, float ancho, float alto, GLuint textura);


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    
    glutInitWindowSize(900, 650);//tam de la ventana
    glutInitWindowPosition(100, 50);
    ventana_principal = glutCreateWindow("Personajes chavas");
    
    glClearColor(0.85, 0.9, 0.95, 1.0); 
    inicializarTexturas();  // Cargar texturas al inici
    //ejemplos de los personajes
    personajes[0] = crearPersonaje("Rubi", 0.0, 0.0, 1.0, 0.55, 0.25, 0.85, 0);// RUBI (Personaje 0 - Morado)
    personajes[1] = crearPersonaje("Roxana", 0.0, 0.0, 1.0, 0.9, 0.2, 0.3, 0);// ROXANA (Personaje 1 - Rojo)
    personajes[3] = crearPersonaje("Renata", 0.0, 0.0, 1.0, 0.3, 0.8, 0.4, 0);// RENATA (Personaje 2 - Verde
    personajes[2] = crearPersonaje("Rebeca", 0.0, 0.0, 1.0, 0.2, 0.4, 0.9, 0);// REBECA (Personaje 3 - Azul)
    numPersonajes = 4;
    
    glutDisplayFunc(display);
    glutIdleFunc(redibujo);
    glutKeyboardFunc(teclado);
    
    // ventana de menu 
    glutInitWindowSize(400, 550);
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
        //se convierten los ang a coor normales
        float px = x + radio * cos(angulo);
        float py = y + radio * sin(angulo);
        glVertex2f(px, py);
    }
    glEnd();
}

//esto es lo que siempre uso en mis programas jejeje
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

// Crea un nodo nuevo para el arbol jerárquico
Nodo* crearNodo(char* nombre, double tx, double ty, double longitud, double angulo) {
    Nodo* nuevo = (Nodo*)malloc(sizeof(Nodo));
    strcpy(nuevo->nombre, nombre);// Copia la cadena 'nombre' dentro del campo nombre del nodo
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
    hijo->padre = padre;                //establece padre del hijo
    if (padre->primerHijo == NULL) {    //padre sin hijos, primer hijo
        padre->primerHijo = hijo;
    } else {
        Nodo* hermano = padre->primerHijo;//recorre lista hermano hasta el ultimo 
        //
        while (hermano->siguienteHermano != NULL) {
            hermano = hermano->siguienteHermano;    //hasta llegar a ultimo hermano
        }
        hermano->siguienteHermano = hijo;//añade hijo como hermano del ult 
    }
}

// libera recursivamente toda la memoria del árbol
void liberarArbol(Nodo* nodo) {
    if (nodo == NULL) return;
    liberarArbol(nodo->primerHijo);
    liberarArbol(nodo->siguienteHermano);
    free(nodo);
}

// Construye el arbol jerárquico completo del personaje
void construirArbolPersonaje(Personaje* p) {
    // calcular longitudes de cada segmento, distancia euclideana , gracias doc Manir
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
    
    // PIERNA IZQUIERDA: Torso - Muslo -Pantorrilla - Pie
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
    actualizarAngulosEnNodos(nodo->primerHijo, p);    // Llamada recursiva para actualizar todos los nodos hijos del nodo actual produndid
    actualizarAngulosEnNodos(nodo->siguienteHermano, p);// Llamada recursiva para actualizar todos los nodos hermanos del nodo actual anchura
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


void dibujarCabezaPersonaje(Personaje* p) {
    // Guardar la matriz actual
    glPushMatrix();
    
    // Mover al centro de la cabeza 
    glTranslatef(centroTorso[0], centroTorso[1] - 0.05, 0);
    
    // Dibujar cabello (parte de atrás)
    glColor3f(0.15, 0.10, 0.08); // Color café oscuro para el cabello
    dibujarCirculo(0, 0.36, 0.095, 30);
    
    // Dibujar cara
    glColor3f(0.95, 0.87, 0.78); // Color piel
    dibujarCirculo(0, 0.36, 0.08, 30);
    
    // Dibujar ojos ovalados
    glColor3f(1, 1, 1); // Blanco de los ojos
    // Ojo izquierdo 
    glPushMatrix();
    glTranslatef(-0.03, 0.37, 0);
    glScalef(1.5, 1.0, 1.0); 
    dibujarCirculo(0, 0, 0.02, 16);
    glPopMatrix();
    
    // Ojo derecho
    glPushMatrix();
    glTranslatef(0.03, 0.37, 0);
    glScalef(1.5, 1.0, 1.0);
    dibujarCirculo(0, 0, 0.02, 16);
    glPopMatrix();
    
    // Pupilas 
    glColor3f(0, 0, 0); 
    // Pupila izquierda
    glPushMatrix();
    glTranslatef(-0.03, 0.37, 0);
    glScalef(1.5, 1.0, 1.0);
    dibujarCirculo(0, 0, 0.01, 12);
    glPopMatrix();
    
    // Pupila derecha
    glPushMatrix();
    glTranslatef(0.03, 0.37, 0);
    glScalef(1.5, 1.0, 1.0);
    dibujarCirculo(0, 0, 0.01, 12);
    glPopMatrix();
    
    // Boca sonriente
    glColor3f(0.8, 0.2, 0.2); // Rojo para la boca
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    for (float i = -0.04; i <= 0.04; i += 0.01) {
        float y = 0.33 - 0.02 * (1 - i*i/0.0016);
        glVertex2f(i, y);
    }
    glEnd();
    

    // Dibujar cuello
    glColor3f(0.95, 0.87, 0.78);
    float cuelloPersona[][2] = {
        {-0.025, 0.22},
        {0.025, 0.22},
        {0.025, 0.26},
        {-0.025, 0.26}
    };
    dibujarPoligono(cuelloPersona, 4);
    
    // Restaurar la matriz
    glPopMatrix();
    
    // Dibujar mechones de cabello que sobresalen
    glPushMatrix();
    glTranslatef(centroTorso[0], centroTorso[1], 0);
    
    // Mechón izquierdo
    float mechonIzq[][2] = {
        {-0.08, 0.36},
        {-0.10, 0.30},
        {-0.11, 0.20},
        {-0.10, 0.19},
        {-0.08, 0.28},
        {-0.075, 0.34}
    };
    glColor3f(0.15, 0.10, 0.08);
    dibujarPoligono(mechonIzq, 6);
    
    // Mechón derecho
    float mechonDer[][2] = {
        {0.08, 0.36},
        {0.10, 0.30},
        {0.11, 0.20},
        {0.10, 0.19},
        {0.08, 0.28},
        {0.075, 0.34}
    };
    dibujarPoligono(mechonDer, 6);
    
    glPopMatrix();
}

// Dibuja un nodo del árbol jerárquico y sus hijos
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

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Renderizar la escena actual de la cola
    renderizarEscenaActual();
    
    // Indicador de información
    glColor3f(0.5, 0.5, 0.5);
    glRasterPos2f(-0.98, 0.95);
    char info[100];
    sprintf(info, "Escena %d | Tiempo: %.1fs", escena_actual, colaEscenas.tiempoTotal);
    for(int i = 0; info[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, info[i]);
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


void dibujarTexto(char *texto, float x, float y) {
    glRasterPos2f(x, y);
    for(int i = 0; texto[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, texto[i]);
    }
}

void dibujarRectangulo(float x, float y, float ancho, float alto) {
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + ancho, y);
        glVertex2f(x + ancho, y + alto);
        glVertex2f(x, y + alto);
    glEnd();
}


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

void redibujo() {
    SLEEP(30);
    
    if(!animaciones_inicializadas) {
        inicializarAnimaciones();
        inicializarEscenas();
    }
    
    if(!animacion_pausada) {
        colaEscenas.tiempoTotal += 0.03;
        tiempo_total += 0.03;
        
        for(int i = 0; i < numPersonajes; i++) {
            if(animaciones[i].numKeyframes > 0) {
                reproducirAnimacion(i);
            }
        }
        
        fantasma_oscilacion += 0.1;
        
        if(escena_actual == 1) {
        if(fantasma_escala_inicio < 2.5) {  
            fantasma_escala_inicio += 0.015;
        }
        
        if(fantasma_escala_inicio > 0.8) {  
            alpha_texto = (fantasma_escala_inicio - 0.8) / 1.7;
            if(alpha_texto > 1.0) alpha_texto = 1.0;
        } else {
            alpha_texto = 0.0;
        }
        } else {
            alpha_texto = 1.0;
        }
        if(escena_actual == 5 && fantasma_acercamiento < 0.0) {
            fantasma_acercamiento += 0.01;
        }
        else if(escena_actual == 16) {
            if(fantasma_alejamiento > 0.3) {  // Se va encogiendo
                fantasma_alejamiento -= 0.012;  // Velocidad de alejamiento
            }
            
            if(fantasma_alejamiento < 1.0) {
                alpha_texto = fantasma_alejamiento / 1.0;
            } else {
                alpha_texto = 1.0;
            }
        }
        else {
            alpha_texto = 1.0;
        }
    }
    glutPostRedisplay();
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
    // Reiniciar animaciones
    for(int i = 0; i < numPersonajes; i++) {
        animaciones[i].tiempoAnimacion = 0.0;
    }
    
        // Reiniciar escenas y variables
        colaEscenas.tiempoTotal = 0.0;
        tiempo_total = 0.0;
        escena_actual = 0;  // CAMBIAR de 1 a 0
        alpha_texto = 0.0;
        fantasma_x = -0.8;
        fantasma_oscilacion = 0.0;
        fantasma_escala_inicio = 0.3;
        fantasma_acercamiento = -1.2;
        fantasma_alejamiento = 2.5;
        
        animacion_pausada = 1;
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
    if(seleccionado) {
        glColor3f(1.0, 1.0, 1.0);  
    } else {
        glColor3f(0.8, 0.8, 0.8);  
    }
    
    glRasterPos2f(x + 0.05, y + alto * 0.6);
    for(int i = 0; texto[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, texto[i]);
    }
}

// Dibuja la interfaz del menú
void displayMenu(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Título
    glColor3f(1.0, 0.6, 0.0);
    glRasterPos2f(-0.75, 0.85);
    char titulo[] = "CONTROL DE ANIMACION";
    for(int i = 0; titulo[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, titulo[i]);
    }
    
    // Botones 
    float y_base = 0.50;
    float espaciado = 0.22;
    
    dibujarBotonMenu(-0.85, y_base, 1.70, 0.18, "REPRODUCIR", opcion_seleccionada == 0);
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
            case 2:  // REINICIAR
                for(int i = 0; i < numPersonajes; i++) {
                    personajes[i]->tiempoAnimacion = 0.0;
                    animaciones[i].tiempoAnimacion = 0.0;  
                }
                colaEscenas.tiempoTotal = 0.0;
                tiempo_total = 0.0;
                escena_actual = 0;  // CAMBIAR de 1 a 0
                alpha_texto = 0.0;
                fantasma_x = -0.8;  // CAMBIAR de -1.2 a -0.8
                fantasma_oscilacion = 0.0;
                fantasma_escala_inicio = 0.3;
                fantasma_acercamiento = -1.2;
                fantasma_alejamiento = 2.5;  // AGREGAR esta línea
                animacion_pausada = 1;
                printf("Animacion REINICIADA\n");
                
                break;
            case 3:  // SALIR
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

// Crear un keyframe copiando el estado actual del personaje
Keyframe* crearKeyframe(float tiempo, Personaje* p) {
    Keyframe* nuevo = (Keyframe*)malloc(sizeof(Keyframe));
    if(nuevo == NULL) {
        printf("Error: No se pudo crear keyframe\n");
        return NULL;
    }
    
    nuevo->tiempo = tiempo;
    
    // Copiar todos los ángulos
    nuevo->anguloBrazoDer = p->anguloBrazoDer;
    nuevo->anguloAntebrazoDer = p->anguloAntebrazoDer;
    nuevo->anguloBrazoIzq = p->anguloBrazoIzq;
    nuevo->anguloAntebrazoIzq = p->anguloAntebrazoIzq;
    nuevo->anguloMusloDer = p->anguloMusloDer;
    nuevo->anguloPantorrillaDer = p->anguloPantorrillaDer;
    nuevo->anguloMusloIzq = p->anguloMusloIzq;
    nuevo->anguloPantorrillaIzq = p->anguloPantorrillaIzq;
    
    // Copiar posición
    nuevo->posX = p->posX;
    nuevo->posY = p->posY;
    
    nuevo->siguiente = NULL;
    
    return nuevo;
}

// Agregar keyframe al final de la lista
void agregarKeyframe(AnimacionLista* anim, Keyframe* kf) {
    if(kf == NULL) return;
    
    if(anim->primerKeyframe == NULL) {
        // Lista vacía
        anim->primerKeyframe = kf;
        anim->keyframeActual = kf;
    } else {
        // Buscar el último
        Keyframe* actual = anim->primerKeyframe;
        while(actual->siguiente != NULL) {
            actual = actual->siguiente;
        }
        actual->siguiente = kf;
    }
    
    anim->numKeyframes++;
}

// Liberar toda la lista de keyframes
void liberarAnimacion(AnimacionLista* anim) {
    Keyframe* actual = anim->primerKeyframe;
    while(actual != NULL) {
        Keyframe* siguiente = actual->siguiente;
        free(actual);
        actual = siguiente;
    }
    anim->primerKeyframe = NULL;
    anim->keyframeActual = NULL;
    anim->numKeyframes = 0;
}



// Buscar entre qué keyframes estamos
Keyframe* buscarKeyframeActual(AnimacionLista* anim) {
    Keyframe* actual = anim->primerKeyframe;
    Keyframe* anterior = NULL;
    
    while(actual != NULL) {
        if(actual->tiempo > anim->tiempoAnimacion) {
            return anterior; // Retornar el keyframe anterior
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    
    return anterior; // Último keyframe
}

//aqui se crean las animaciones, (crea los jeyframes y los guarda)
void inicializarAnimaciones() {
    if(animaciones_inicializadas){
        return;
    }
    
    for(int i = 0; i < 10; i++) {
        animaciones[i].primerKeyframe = NULL;
        animaciones[i].keyframeActual = NULL;
        animaciones[i].numKeyframes = 0;
        animaciones[i].tiempoAnimacion = 0.0;
    }

    // ANIMACIÓN 0: Rubi levanta brazos 
    //se crean personajes temporales para diseñar, no son lo que s emuestran
    Personaje* temp0 = crearPersonaje("Temp", 0.0, 0.0, 1.0, 0.55, 0.25, 0.85, 0);
    temp0->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp0->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf0_1 = crearKeyframe(0.0, temp0);
    agregarKeyframe(&animaciones[0], kf0_1);
    
    temp0->anguloBrazoDer = BRAZO_MAX * DEG_TO_RAD * 0.6;
    temp0->anguloAntebrazoDer = ANTEBRAZO_MAX * DEG_TO_RAD * 0.4;   
    Keyframe* kf0_2 = crearKeyframe(2.0, temp0);
    agregarKeyframe(&animaciones[0], kf0_2);
    
    temp0->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.6;
    temp0->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 0.4;
    Keyframe* kf0_3 = crearKeyframe(4.0, temp0);
    agregarKeyframe(&animaciones[0], kf0_3);
    
    temp0->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp0->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    temp0->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    temp0->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf0_4 = crearKeyframe(6.0, temp0);
    agregarKeyframe(&animaciones[0], kf0_4);
    
    eliminarPersonaje(temp0);
    
    // ANIMACIÓN 1: Roxana camina 
    Personaje* temp1 = crearPersonaje("Temp", -0.6, 0.0, 0.8, 0.9, 0.2, 0.3, 0);

    // Keyframe 1
    temp1->posX = -0.4;
    temp1->anguloMusloDer = MUSLO_MIN * DEG_TO_RAD;
    temp1->anguloMusloIzq = MUSLO_MAX * DEG_TO_RAD * 0.3;
    Keyframe* kf1_1 = crearKeyframe(0.0, temp1);
    agregarKeyframe(&animaciones[1], kf1_1);

    // Keyframe 2
    temp1->posX = 0.0;
    temp1->anguloMusloDer = MUSLO_MAX * DEG_TO_RAD * 0.3;
    temp1->anguloMusloIzq = MUSLO_MIN * DEG_TO_RAD;
    Keyframe* kf1_2 = crearKeyframe(3.0, temp1);
    agregarKeyframe(&animaciones[1], kf1_2);

    // Keyframe 3
    temp1->posX = 0.4;
    temp1->anguloMusloDer = MUSLO_MIN * DEG_TO_RAD;
    temp1->anguloMusloIzq = MUSLO_MAX * DEG_TO_RAD * 0.3;
    Keyframe* kf1_3 = crearKeyframe(6.0, temp1);
    agregarKeyframe(&animaciones[1], kf1_3);

    temp1->posX = 0.4;  // MISMA posición que keyframe 3
    temp1->anguloMusloDer = MUSLO_MIN * DEG_TO_RAD;  // Piernas rectas
    temp1->anguloMusloIzq = MUSLO_MIN * DEG_TO_RAD;  // Piernas rectas
    Keyframe* kf1_4 = crearKeyframe(8.0, temp1);  
    agregarKeyframe(&animaciones[1], kf1_4);

    

    eliminarPersonaje(temp1);
    

    //ANIMACION 2 - Mano en el pecho
    Personaje* temp2 = crearPersonaje("Temp", 0.0, 0.0, 0.8, 0.2, 0.4, 0.9, 0);   

    // Keyframe 1: Brazos abajo (posición inicial)
    temp2->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp2->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf2_1 = crearKeyframe(0.0, temp2);
    agregarKeyframe(&animaciones[2], kf2_1);

    // Keyframe 2: Mano izquierda subiendo hacia el pecho
    temp2->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.8;      // Levantar brazo izquierdo
    temp2->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 0.9;  // Doblar antebrazo hacia el pecho
    // Brazo derecho se queda abajo
    temp2->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp2->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf2_2 = crearKeyframe(1.5, temp2);
    agregarKeyframe(&animaciones[2], kf2_2);

    // Keyframe 3: Mano izquierda completamente en el pecho (corazón)
    temp2->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.55;     
    temp2->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 0.9; // Más doblado
    Keyframe* kf2_3 = crearKeyframe(3.0, temp2);
    agregarKeyframe(&animaciones[2], kf2_3);

    // Keyframe 4: Mantener mano en el pecho (pose sostenida)
    Keyframe* kf2_4 = crearKeyframe(7.0, temp2);  // Mantener pose más tiempo
    agregarKeyframe(&animaciones[2], kf2_4);

    // Keyframe 5: Bajar mano izquierda
    temp2->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    temp2->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD*1.0;
    temp2->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp2->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf2_5 = crearKeyframe(10.0, temp2);
    agregarKeyframe(&animaciones[2], kf2_5);

    eliminarPersonaje(temp2);


    //animacion 3
  
    Personaje* temp3 = crearPersonaje("Temp", 0.0, 0.0, 1.0, 0.3, 0.8, 0.4, 0);

    // Brazos abajo
    temp3->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp3->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf3_1 = crearKeyframe(0.0, temp3);
    agregarKeyframe(&animaciones[3], kf3_1);

    // Levantar ambos brazos al frente
    temp3->anguloBrazoDer = BRAZO_MAX * DEG_TO_RAD * 0.8;
    temp3->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.5;
    temp3->anguloAntebrazoDer = ANTEBRAZO_MAX * DEG_TO_RAD * 0.6;
    temp3->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 0.6;
    Keyframe* kf3_2 = crearKeyframe(1.0, temp3);
    agregarKeyframe(&animaciones[3], kf3_2);

    // Mantener posición (brazos cruzados)
    Keyframe* kf3_3 = crearKeyframe(3.0, temp3);
    agregarKeyframe(&animaciones[3], kf3_3);

    // Volver a la normalidad
    temp3->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp3->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    temp3->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    temp3->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf3_4 = crearKeyframe(4.0, temp3);
    agregarKeyframe(&animaciones[3], kf3_4);
    eliminarPersonaje(temp3);

    // ANIMACIÓN 4: Rubi se lleva las manos a la cara (escena computadora)
    Personaje* temp4 = crearPersonaje("Temp", 0.0, 0.0, 1.0, 0.55, 0.25, 0.85, 0);

    // Keyframe 1: Brazos abajo (posición inicial)
    temp4->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp4->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    temp4->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    temp4->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf4_1 = crearKeyframe(0.0, temp4);
    agregarKeyframe(&animaciones[4], kf4_1);

    // Keyframe 2: Levantando brazos hacia la cara (más al centro)
    temp4->anguloBrazoDer = BRAZO_MAX * DEG_TO_RAD * 0.95;      // Brazo derecho más levantado
    temp4->anguloAntebrazoDer = ANTEBRAZO_MAX * DEG_TO_RAD * 1.0;  // Antebrazo completamente doblado
    temp4->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.75;       // Brazo izquierdo menos levantado (más hacia la izq)
    temp4->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 1.0;  // Antebrazo completamente doblado hacia dentro
    Keyframe* kf4_2 = crearKeyframe(1.5, temp4);
    agregarKeyframe(&animaciones[4], kf4_2);

    // Keyframe 3: Manos completamente en la cara (pose final centrada)
    temp4->anguloBrazoDer = BRAZO_MAX * DEG_TO_RAD * 1.0;        // Brazo derecho totalmente levantado
    temp4->anguloAntebrazoDer = ANTEBRAZO_MAX * DEG_TO_RAD * 1.0;
    temp4->anguloBrazoIzq = BRAZO_MAX * DEG_TO_RAD * 0.70;       // Brazo izquierdo flexionado hacia la izquierda
    temp4->anguloAntebrazoIzq = ANTEBRAZO_MAX * DEG_TO_RAD * 1.0;
    Keyframe* kf4_3 = crearKeyframe(3.0, temp4);
    agregarKeyframe(&animaciones[4], kf4_3);

    // Keyframe 4: Mantener manos en la cara (pose sostenida)
    Keyframe* kf4_4 = crearKeyframe(7.0, temp4);  // Mantener más tiempo
    agregarKeyframe(&animaciones[4], kf4_4);

    // Keyframe 5: Bajar las manos lentamente
    temp4->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
    temp4->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
    temp4->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
    temp4->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
    Keyframe* kf4_5 = crearKeyframe(9.0, temp4);
    agregarKeyframe(&animaciones[4], kf4_5);
    eliminarPersonaje(temp4);
    animaciones_inicializadas = 1;
}
int ultima_escena_ejecutada = 0;

//Decide que animacion va a usar
void reproducirAnimacion(int indicePersonaje) {
    if(indicePersonaje < 0 || indicePersonaje >= numPersonajes){ 
        return;
    }
    
    int indiceAnimacion = indicePersonaje; // Por defecto usa su propia animación
    int debe_animar = 0; // Flag para saber si debe animar en esta escena
    
    // Decidir qué animación usar según la escena actual
    if(escena_actual == 2) {
        // ESCENA 2: Altar de muertos
        debe_animar = 1;
        if(indicePersonaje == 0) indiceAnimacion = 0;      
        else if(indicePersonaje == 1) indiceAnimacion = 1; 
        else if(indicePersonaje == 2) indiceAnimacion = 2; 
        else if(indicePersonaje == 3) indiceAnimacion = 0;
    }
    else if(escena_actual == 4) {
        debe_animar = 1;
        if(indicePersonaje == 0) indiceAnimacion = 0;      
        else if(indicePersonaje == 1) indiceAnimacion = 0; 
        else if(indicePersonaje == 2) indiceAnimacion = 2; 
        else if(indicePersonaje == 3) indiceAnimacion = 0; 
    }
    else if(escena_actual == 7) {
        if(indicePersonaje == 3) {
            debe_animar = 1;
            indiceAnimacion = 3;
        }
    }
    else if(escena_actual == 8) {
        if(indicePersonaje == 0) {
            debe_animar = 1;
            indiceAnimacion = 2;
        }
    }
    else if(escena_actual == 9) {
        if(indicePersonaje == 2) {
            debe_animar = 1;
            indiceAnimacion = 1;
        }
    }
    else if(escena_actual == 10) {
        if(indicePersonaje == 0) {
            debe_animar = 1;
            indiceAnimacion = 4;
        }
    }
    
    // Si cambió la escena, reiniciar el tiempo de animación
    if(escena_actual != ultima_escena_ejecutada) {
        for(int i = 0; i < numPersonajes; i++) {
            animaciones[i].tiempoAnimacion = 0.0;
        }
        ultima_escena_ejecutada = escena_actual;
    }
    
    // Si no debe animar en esta escena, salir
    if(!debe_animar) {
        return;
    }

    AnimacionLista* anim = &animaciones[indiceAnimacion];
    Personaje* p = personajes[indicePersonaje];
    
    // Incrementar tiempo de animación
    anim->tiempoAnimacion += 0.05;
    
    // Buscar entre qué keyframes estamos
    Keyframe* k1 = buscarKeyframeActual(anim);
    if(k1 == NULL) {
        // Si no hay keyframes, reiniciar
        anim->tiempoAnimacion = 0.0;
        return;
    }
    
    Keyframe* k2 = k1->siguiente;
    if(k2 == NULL) {
        // Llegamos al último keyframe - MANTENER LA POSE FINAL
        // Copiar los ángulos del último keyframe
        p->anguloBrazoDer = k1->anguloBrazoDer;
        p->anguloAntebrazoDer = k1->anguloAntebrazoDer;
        p->anguloBrazoIzq = k1->anguloBrazoIzq;
        p->anguloAntebrazoIzq = k1->anguloAntebrazoIzq;
        p->anguloMusloDer = k1->anguloMusloDer;
        p->anguloPantorrillaDer = k1->anguloPantorrillaDer;
        p->anguloMusloIzq = k1->anguloMusloIzq;
        p->anguloPantorrillaIzq = k1->anguloPantorrillaIzq;
        p->posX = k1->posX;
        p->posY = k1->posY;
        
        actualizarAngulosPersonaje(p);
        return;
    }
    
    // Interpolar entre k1 y k2
    float duracion = k2->tiempo - k1->tiempo;
    float tiempoLocal = anim->tiempoAnimacion - k1->tiempo;
    float t = tiempoLocal / duracion;
    
    interpolarKeyframes(k1, k2, t, p);
    actualizarAngulosPersonaje(p);
}
// Interpolar entre dos keyframes
void interpolarKeyframes(Keyframe* k1, Keyframe* k2, float t, Personaje* p) {
    if(k1 == NULL || k2 == NULL) return;
    
    if(t < 0.0) t = 0.0;
    if(t > 1.0) t = 1.0;
    
    // Interpolar BRAZOS
    p->anguloBrazoDer = k1->anguloBrazoDer + t * (k2->anguloBrazoDer - k1->anguloBrazoDer);
    p->anguloAntebrazoDer = k1->anguloAntebrazoDer + t * (k2->anguloAntebrazoDer - k1->anguloAntebrazoDer);
    p->anguloBrazoIzq = k1->anguloBrazoIzq + t * (k2->anguloBrazoIzq - k1->anguloBrazoIzq);
    p->anguloAntebrazoIzq = k1->anguloAntebrazoIzq + t * (k2->anguloAntebrazoIzq - k1->anguloAntebrazoIzq);
    
    // Interpolar PIERNAS
    p->anguloMusloDer = k1->anguloMusloDer + t * (k2->anguloMusloDer - k1->anguloMusloDer);
    p->anguloPantorrillaDer = k1->anguloPantorrillaDer + t * (k2->anguloPantorrillaDer - k1->anguloPantorrillaDer);
    p->anguloMusloIzq = k1->anguloMusloIzq + t * (k2->anguloMusloIzq - k1->anguloMusloIzq);
    p->anguloPantorrillaIzq = k1->anguloPantorrillaIzq + t * (k2->anguloPantorrillaIzq - k1->anguloPantorrillaIzq);
    
    // Interpolar POSICIÓN
    p->posX = k1->posX + t * (k2->posX - k1->posX);
    p->posY = k1->posY + t * (k2->posY - k1->posY);
}

// IMPLEMENTACIÓN DE COLAS
// Encolar una escena al final de la cola
void encolarEscena(int num, float inicio, float fin, char* nombre, void (*func)(void)) {
    Escena* nueva = (Escena*)malloc(sizeof(Escena));
    if(nueva == NULL) {
        return;
    }
    
    nueva->numero = num;
    nueva->duracionInicio = inicio;
    nueva->duracionFin = fin;
    strcpy(nueva->nombre, nombre);
    nueva->renderizar = func;
    nueva->siguiente = NULL;
    
    // Si la cola está vacía
    if(colaEscenas.frente == NULL) {
        colaEscenas.frente = nueva;
        colaEscenas.final = nueva;
    } 
    // Si ya hay escenas
    else {
        colaEscenas.final->siguiente = nueva;
        colaEscenas.final = nueva;
    }
    
    colaEscenas.totalEscenas++;
}

// Liberar toda la cola
void liberarCola() {
    Escena* actual = colaEscenas.frente;
    while(actual != NULL) {
        Escena* siguiente = actual->siguiente;
        free(actual);
        actual = siguiente;
    }
    colaEscenas.frente = NULL;
    colaEscenas.final = NULL;
    colaEscenas.totalEscenas = 0;
}

// Renderizar la escena correspondiente al tiempo actual
void renderizarEscenaActual() {
    Escena* actual = colaEscenas.frente;
    float t = colaEscenas.tiempoTotal;
    
    while(actual != NULL) {
        if(t >= actual->duracionInicio && t < actual->duracionFin) {
            escena_actual = actual->numero;
            
            if(actual->renderizar != NULL) {
                actual->renderizar();
            }
            return;
        }
        actual = actual->siguiente;
    }
    
    colaEscenas.tiempoTotal = 0.0;
    tiempo_total = 0.0;
    escena_actual = 0;  
    alpha_texto = 0.0;
    fantasma_x = -0.8;
    fantasma_oscilacion = 0.0;
    fantasma_escala_inicio = 0.3;
    fantasma_acercamiento = -1.2;
    fantasma_alejamiento = 2.5;  // Agregar si no está

    for(int i = 0; i < numPersonajes; i++) {
        animaciones[i].tiempoAnimacion = 0.0;
    }
}


void escena1_presentacion() {
    if(texturas_cargadas > 0 && texInicioMictlan.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaInicioMictlan);
    }    
   
    float offset_y = sin(fantasma_oscilacion) * 0.06;
    dibujarFantasma(fantasma_x, fantasma_y + offset_y, fantasma_escala_inicio);
    
    
    if(alpha_texto > 0.3) {
        glColor3f(1.0, 1.0, 1.0);
        glRasterPos2f(-0.90, -0.80);//(X,Y)
        char texto1[] = "Juntos de nuevo,";
        for(int i = 0; texto1[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto1[i]);
        }
        
        glRasterPos2f(-0.90, -0.88);
        char texto2[] = "aunque sea un ratito";
        for(int i = 0; texto2[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto2[i]);
        }
    }
}



void escena2_altar() {
    if(texturas_cargadas >= 2 && texFondoSala.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoSala);
    }
    
    if(texturas_cargadas >= 3 && texAltar.data != NULL) {
        float altar_ancho = 0.7;
        float altar_alto = 0.65; 
        float altar_x = -altar_ancho / 2.0;
        float altar_y = -0.5;
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        dibujarRectanguloTexturizado(altar_x, altar_y, altar_ancho, altar_alto, texturaAltar);
        glDisable(GL_BLEND);
    }
    
    glPushMatrix();
        glTranslatef(0.0, -0.2, 0.0);
        glPushMatrix();
            glTranslatef(-0.60, 0.0, 0.0);//pos de la roja
            glScalef(0.9, 0.9, 1.0);
            dibujarPersonaje(personajes[1]);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(-0.80, 0.0, 0.0);//posicion de la morada
            glScalef(1.2, 1.2, 1.0);//(ancho, alto)
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        
        
        glPushMatrix();
            glTranslatef(0.75, 0.2, 0.0);//pos de la azul
            glScalef(0.7, 0.7, 1.0);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
        glPushMatrix();
            glTranslatef(0.50, 0.0, 0.0);//pos de la verde,     quieta
            glScalef(0.9, 0.9, 1.0);
            double temp3_brazoDer = personajes[3]->anguloBrazoDer;
            double temp3_antebrazoDer = personajes[3]->anguloAntebrazoDer;
            double temp3_brazoIzq = personajes[3]->anguloBrazoIzq;
            double temp3_antebrazoIzq = personajes[3]->anguloAntebrazoIzq;
            
            personajes[3]->anguloBrazoDer = BRAZO_MIN * DEG_TO_RAD;
            personajes[3]->anguloAntebrazoDer = ANTEBRAZO_MIN * DEG_TO_RAD;
            personajes[3]->anguloBrazoIzq = BRAZO_MIN * DEG_TO_RAD;
            personajes[3]->anguloAntebrazoIzq = ANTEBRAZO_MIN * DEG_TO_RAD;
            
            actualizarAngulosPersonaje(personajes[3]);
            dibujarPersonaje(personajes[3]);
            
            personajes[3]->anguloBrazoDer = temp3_brazoDer;
            personajes[3]->anguloAntebrazoDer = temp3_antebrazoDer;
            personajes[3]->anguloBrazoIzq = temp3_brazoIzq;
            personajes[3]->anguloAntebrazoIzq = temp3_antebrazoIzq;
        glPopMatrix();
        
    glPopMatrix();
    
    float offset_y = sin(fantasma_oscilacion) * 0.03;
    dibujarFantasma(0.0, -0.70 + offset_y, 2.0);
    
    if(alpha_texto > 0.5) {
        glColor3f(0.0, 0.0, 0.0);
        glRasterPos2f(-0.40, 0.85);
        char texto[] = "Recordando juntos...";
        for(int i = 0; texto[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto[i]);
        }
    }
}
void escena3_dialogo() {
    if(texturas_cargadas >= 7 && texFondoFantasma.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoFantasma);
    } 

    float offset_y = sin(fantasma_oscilacion) * 0.04;
    dibujarFantasma(0.0, 0.0 + offset_y, 4.0);
    
    // Texto
    glColor3f(1.0, 1.0, 1.0);
    if(alpha_texto > 0.3) {
        dibujarTexto("Que bueno volver a ver a mis chavas", -0.35, -0.6);
    }
}

void escena4_despedida() {
    // Fondo
    if(texturas_cargadas >= 2 && texFondoEsc4.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoEsc4);
    }

    // AHORA CADA PERSONAJE TIENE SU PROPIA POSICIÓN Y ESCALA
    glPushMatrix();
        glTranslatef(0.0, -0.2, 0.0); // Ajuste general de altura
        
        // RUBI (Personaje 0 - Morado)
        glPushMatrix();
            glTranslatef(0.80, 0.0, 0.0);  // Posición izquierda
            glScalef(0.9, 0.9, 1.0);         // Escala
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        // ROXANA (Personaje 1 - Rojo)
        glPushMatrix();
            glTranslatef(-0.25, 0.2, 0.0);   // Posición centro-izq
            glScalef(0.6, 0.6, 1.0);
            dibujarPersonaje(personajes[1]);
        glPopMatrix();
        
        // REBECA (Personaje 2 - Azul)
        glPushMatrix();
            glTranslatef(-0.80, 0.0, 0.0);    // Posición centro-der
            glScalef(0.9, 0.9, 1.0);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
        // RENATA (Personaje 3 - Verde)
        glPushMatrix();
            glTranslatef(0.25, 0.2, 0.0);    // Posición derecha
            glScalef(0.6, 0.6, 1.0);
            dibujarPersonaje(personajes[3]);
        glPopMatrix();
        
    glPopMatrix();
    
    // Textos
    glColor3f(1.0, 1.0, 1.0);
    if(alpha_texto > 0.3) {
        dibujarTexto("Me encanta ponerle ofrenda,", -0.28, -0.7);
        dibujarTexto("pero preferiria no tener que hacerlo :')", -0.38, -0.8);
    }
}

void escena5_fantasma_decision(void) {
    if(texturas_cargadas >= 2 && texFondoSala.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoSala);
    }

     // RUBI (Personaje 0 - Morado)
        glPushMatrix();
            glTranslatef(0.80, 0.0, 0.0);  // Posición izquierda
            glScalef(0.9, 0.9, 1.0);         // Escala
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        
        // REBECA (Personaje 2 - Azul)
        glPushMatrix();
            glTranslatef(-0.80, 0.0, 0.0);    // Posición centro-der
            glScalef(0.9, 0.9, 1.0);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
            
    glPopMatrix();

    

    float offset_y = sin(fantasma_oscilacion) * 0.03;
    dibujarFantasma(fantasma_acercamiento, -0.6 + offset_y, 2.2);
    
    glColor3f(0.0, 0.0, 0.0);
    if(alpha_texto > 0.3) {
        dibujarTexto("*Quiero quedarme mas tiempo con ellas!", fantasma_acercamiento - 0.35, 0.0);
        dibujarTexto("*Si me quedo, no creo que noten que estoy aqui!", fantasma_acercamiento - 0.45, -0.10);
    }
}

void escena6_transicion_tiempo(void) {
    if(texturas_cargadas >= 7 && texFondoNoche.data != NULL)  {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoNoche);
    }
    
    glColor3f(1.0, 1.0, 1.0);
    if(alpha_texto > 0.3) {
        glRasterPos2f(0.30, 0.0);
        char texto[] = "2 Semanas despues...";
        for(int i = 0; texto[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto[i]);
        }
    }
}


void escena7_mujer_verde_cuarto(void) {
    if(texturas_cargadas >= 4 && texfondoEsc7.data != NULL)  {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturafondoEsc7);
    } 
   
    float offset_y = sin(fantasma_oscilacion) * 0.02;
    dibujarFantasma(0.70, -0.50 + offset_y, 2.0);
    
    glPushMatrix();
        glTranslatef(0.0, -0.45, 0.0);
        glScalef(1.5, 1.5, 1.0);
        dibujarPersonaje(personajes[3]);
    glPopMatrix();
    
    glColor3f(0.2, 0.2, 0.2);
    if(alpha_texto > 0.3) {
        dibujarTexto("*Aun puedo sentir como si", -0.95, 0.85);
        dibujarTexto("estuvieras aqui", -0.95, 0.78);
    }
}

void escena8_mujer_rosa_oficina(void) {

    if(texturas_cargadas >= 6 && texFondoOficina.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoOficina);
    } 
    
    float offset_y = sin(fantasma_oscilacion) * 0.02;
    dibujarFantasma(-0.75, -0.60 + offset_y, 2.0);
    
    glPushMatrix();
        glTranslatef(0.0, -0.45, 0.0);
        glScalef(1.2, 1.2, 1.0);
        dibujarPersonaje(personajes[0]);
    glPopMatrix();
    
    glColor3f(0.2, 0.2, 0.2);
    if(alpha_texto > 0.3) {
        dibujarTexto("Se que te hubieras alegrado", -0.95, 0.85);
        dibujarTexto("si supieras en que trabajo estoy", -0.95, 0.78);
    }
}

void escena9_mujer_azul_graduacion(void) {
    if(texturas_cargadas >= 5) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoGraduacion);
    } 
    
    float offset_y = sin(fantasma_oscilacion) * 0.02;
    dibujarFantasma(-0.50, -0.40 + offset_y, 0.65);
    
    glPushMatrix();
        glTranslatef(-0.50, -0.25, 0.0);    
        glScalef(1.2, 1.2, 1.0);          
        dibujarPersonaje(personajes[2]);  
    glPopMatrix();
    
    glColor3f(0.0, 0.0, 0.0);
    if(alpha_texto > 0.3) {
        dibujarTexto("*Que daria por que", -0.95, 0.85);
        dibujarTexto("estuvieras aqui", -0.95, 0.78);
    }
}

void escena10_mujer_morado_computadora(void) {
    if(texturas_cargadas >= 4 && texFondoCuarto.data != NULL)  {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoCuarto);
    } 
    float offset_y = sin(fantasma_oscilacion) * 0.02;
    dibujarFantasma(-0.55, -0.45 + offset_y, 2.0);
    
    glPushMatrix();
        glTranslatef(0.05, -0.45, 0.0);
        glScalef(1.2, 1.2, 1.0);
        dibujarPersonaje(personajes[0]);
    glPopMatrix();
    
    glColor3f(0.2, 0.2, 0.2);
    if(alpha_texto > 0.3) {
        dibujarTexto("Nunca entendiste que estudiaba", -0.95, 0.85);
        dibujarTexto("pero se que estabas feliz por mi", -0.95, 0.78);
    }
}


void escena11_transicion_navidad(void) {
    if(texturas_cargadas >= 2) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoNoche);
    }
    
    glColor3f(1.0, 1.0, 1.0);
    if(alpha_texto > 0.3) {
        glRasterPos2f(0.25, 0.0);
        char texto[] = "1 Semana despues...";
        for(int i = 0; texto[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto[i]);
        }
    }
}

void escena12_navidad_recuerdos(void) {
    if(texturas_cargadas >= 6 && texFondoArbol.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoArbol);
    } 
    
    float offset_y = sin(fantasma_oscilacion) * 0.03;
    dibujarFantasma(0.0, -0.2 + offset_y, 1.0);
    
    glPushMatrix();
        glTranslatef(0.0, -0.2, 0.0); // Ajuste general de altura
        
        // RUBI (Personaje 0 - Morado)
        glPushMatrix();
            glTranslatef(0.80, 0.0, 0.0);  // Posición izquierda
            glScalef(0.9, 0.9, 1.0);         // Escala
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        // ROXANA (Personaje 1 - Rojo)
        glPushMatrix();
            glTranslatef(-0.25, 0.2, 0.0);   // Posición centro-izq
            glScalef(0.6, 0.6, 1.0);
            dibujarPersonaje(personajes[1]);
        glPopMatrix();
        
        // REBECA (Personaje 2 - Azul)
        glPushMatrix();
            glTranslatef(-0.90, 0.0, 0.0);    // Posición centro-der
            glScalef(0.9, 0.9, 1.5);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
        // RENATA (Personaje 3 - Verde)
        glPushMatrix();
            glTranslatef(0.25, -0.2, 0.0);    // Posición derecha
            glScalef(1.0, 1.0, 3.0);
            dibujarPersonaje(personajes[3]);
        glPopMatrix();
        
    glPopMatrix();
    
    glColor3f(0.2, 0.2, 0.2);
    if(alpha_texto > 0.3) {
        dibujarTexto("Se acuerdan cuando los 5 fuimos", -0.95, 0.75);
        dibujarTexto("a ese viaje a la playa?", -0.95, 0.68);
    }
    
    if(alpha_texto > 0.6) {
        dibujarTexto("Ay siii, fue el mejor viaje,", -0.95, 0.55);
        dibujarTexto("en especial porque el estaba", -0.95, 0.48);
    }
}

void escena13_cena_navidad(void) {
   if(texturas_cargadas >= 10 && texFondoCenaNavidad.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoCenaNavidad);
    } 

    float offset_y = sin(fantasma_oscilacion) * 0.02;
    dibujarFantasma(-0.8, -0.75 + offset_y, 1.5);
    
    

 glPushMatrix();
        glTranslatef(0.0, -0.2, 0.0); 
        
        // RUBI (Personaje 0 - Morado)
        glPushMatrix();
            glTranslatef(-0.55, 0.0, 0.0);  
            glScalef(0.9, 0.9, 1.0);         // Escala
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        // ROXANA (Personaje 1 - Rojo)
        glPushMatrix();
            glTranslatef(-0.20, 0.0, 0.0);  
            glScalef(0.6, 0.6, 1.0); 
            dibujarPersonaje(personajes[1]);
        glPopMatrix();
        
        // REBECA (Personaje 2 - Azul)
        glPushMatrix();
            glTranslatef(0.90, 0.0, 0.0);  
            glScalef(0.9, 0.9, 1.5);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
        // RENATA (Personaje 3 - Verde)
        glPushMatrix();
            glTranslatef(0.55, 0.0, 0.0);
            glScalef(1.0, 1.0, 3.0);
            dibujarPersonaje(personajes[3]);
        glPopMatrix();
        
    glPopMatrix();

    glColor3f(0.2, 0.2, 0.2);
    if(alpha_texto > 0.2) {
        dibujarTexto("*Que rico quedo el espagueti!", -0.95, 0.65);
    }
    if(alpha_texto > 0.4) {
        dibujarTexto("*La pierna tambien nos quedó genial", -0.95, 0.55);
    }
    if(alpha_texto > 0.6) {
        dibujarTexto("*No sienten una calidez como si", -0.95, 0.45);
        dibujarTexto("el estuviera aqui?", -0.95, 0.38);
    }
}

void escena14_fantasma_despedida(void) {
    
    if(texturas_cargadas >= 6 && texFondoArbol.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturaFondoArbol);
    } 

    float offset_y = sin(fantasma_oscilacion) * 0.04;
    dibujarFantasma(0.0, 0.0 + offset_y, 4.0);
    
    dibujarGorroNavidad(0.0, 0.35 + offset_y, 4.0);
    
    glColor3f(0.0, 0.0, 0.0);
    if(alpha_texto > 0.3) {
        glRasterPos2f(-0.35, -0.55);
        char mensaje[] = "Este siempre sera nuestro dia";
        for(int i = 0; mensaje[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, mensaje[i]);
        }
    }
    
    // Corazones flotando
    if(alpha_texto > 0.5) {
        glColor3f(1.0, 0.3, 0.5);
        for(int i = 0; i < 6; i++) {
            float x = -0.6 + i * 0.25;
            float y = -0.75 + sin(tiempo_total * 2 + i) * 0.08;
            
            // Corazón simple
            dibujarCirculo(x - 0.02, y + 0.02, 0.025, 15);
            dibujarCirculo(x + 0.02, y + 0.02, 0.025, 15);
            glBegin(GL_TRIANGLES);
                glVertex2f(x - 0.04, y + 0.01);
                glVertex2f(x + 0.04, y + 0.01);
                glVertex2f(x, y - 0.04);
            glEnd();
        }
    }
}

void escena15_despedida_hijas(void) {
    if(texturas_cargadas >= 4 && texfondoEsc7.data != NULL)  {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturafondoEsc7);
    } 
    
    float offset_y = sin(fantasma_oscilacion) * 0.03;
    dibujarFantasma(0.0, 0.55 + offset_y, 1.5);


    glPushMatrix();
        glTranslatef(0.0, -0.2, 0.0); // Ajuste general de altura
        
        // RUBI (Personaje 0 - Morado)
        glPushMatrix();
            glTranslatef(-0.65, 0.0, 0.0);  
            glScalef(0.9, 0.9, 1.0);         // Escala
            dibujarPersonaje(personajes[0]);
        glPopMatrix();
        
        // ROXANA (Personaje 1 - Rojo)
        glPushMatrix();
            glTranslatef(-0.22, 0.0, 0.0);   
            glScalef(0.6, 0.6, 1.0);
            dibujarPersonaje(personajes[1]);
        glPopMatrix();
        
        // REBECA (Personaje 2 - Azul)
        glPushMatrix();
            glTranslatef(0.1, 0.0, 0.0);    
            glScalef(0.9, 0.9, 1.5);
            dibujarPersonaje(personajes[2]);
        glPopMatrix();
        
        // RENATA (Personaje 3 - Verde)
        glPushMatrix();
            glTranslatef(0.65, 0.0, 0.0);    
            glScalef(1.0, 1.0, 3.0);
            dibujarPersonaje(personajes[3]);
        glPopMatrix();
        
    glPopMatrix();
    
    glColor3f(0.0, 0.0, 0.0);
    if(alpha_texto > 0.2) {
        dibujarTexto("*Pa sabemos que estas aqui con nosotras,", -0.95, 0.90);
        dibujarTexto("debes de irte", -0.95, 0.85);
    }
    if(alpha_texto > 0.4) {
        dibujarTexto("*Estaremos bien, aunque te extranemos,", -0.95, 0.70);
        dibujarTexto("tu ya no debes de estar aqui", -0.95, 0.65);
    }
    if(alpha_texto > 0.6) {
        dibujarTexto("*Te amaremos por siempre", -0.95, 0.50);
    }
    
    if(alpha_texto > 0.8) {
        glColor3f(0.0, 0.0, 0.0);
        glRasterPos2f(-0.25, 0.38);
        char respuesta[] = "Las amare por siempre";
        for(int i = 0; respuesta[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, respuesta[i]);
        }
    }
}

void escena16_camino_mictlan(void) {
   if(texturas_cargadas >= 9 && texcaminoMictlan.data != NULL) {
        dibujarRectanguloTexturizado(-1.0, -1.0, 2.0, 2.0, texturacaminoMictlan);
    } 
    
    
    float offset_y = sin(fantasma_oscilacion) * 0.02;
    float escala_fantasma = 0.8 - (fantasma_acercamiento * 0.3);
   float pos_y = -0.15 + offset_y + (2.5 - fantasma_alejamiento) * 0.2;
    
    dibujarFantasma(0.0, pos_y, fantasma_alejamiento);
    
    if(alpha_texto > 0.5) {
        glColor3f(1.0, 0.9, 0.7);
        glRasterPos2f(-0.48, -0.85);
        char texto[] = "Hasta que nos volvamos a encontrar...";
        for(int i = 0; texto[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, texto[i]);
        }
    }
}



void inicializarEscenas() {
    // encolarEscena(número, INICIO, FIN, "nombre", función);
    encolarEscena(1, 0.0, 10.0, "Fantasma llegando", &escena1_presentacion);
    encolarEscena(2, 10.0, 25.0, "Altar de muertos", &escena2_altar);
    encolarEscena(3, 25.0, 35.0, "Fantasma habla", &escena3_dialogo);
    encolarEscena(4, 35.0, 45.0, "Mujeres hablan", &escena4_despedida);
    encolarEscena(5, 45.0, 55.0, "Fantasma decision", &escena5_fantasma_decision);
    encolarEscena(6, 55.0, 60.0, "2 semanas despues...", &escena6_transicion_tiempo);
    
    encolarEscena(7, 60.0, 70.0, "Mujer verde cuarto", &escena7_mujer_verde_cuarto);
    encolarEscena(8, 70.0, 80.0, "Mujer rosa oficina", &escena8_mujer_rosa_oficina);
    encolarEscena(9, 80.0, 90.0, "Mujer azul graduacion", &escena9_mujer_azul_graduacion);
    encolarEscena(10, 90.0, 100.0, "Mujer morado computadora", &escena10_mujer_morado_computadora);
    
    encolarEscena(11, 100.0, 105.0, "1 semana despues...", &escena11_transicion_navidad);
    
    encolarEscena(12, 105.0, 125.0, "Navidad recuerdos", &escena12_navidad_recuerdos);
    encolarEscena(13, 125.0, 145.0, "Cena navidad", &escena13_cena_navidad);
    
    encolarEscena(14, 145.0, 157.0, "Fantasma despedida", &escena14_fantasma_despedida);
    encolarEscena(15, 157.0, 169.0, "Hijas despedida", &escena15_despedida_hijas);
    encolarEscena(16, 169.0, 180.0, "Camino Mictlan", &escena16_camino_mictlan); 
   
}

unsigned char* cargarBMP(const char* filename, int* width, int* height) {
    FILE* file;
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    unsigned char* data;
    int padding;
    int row;

    // Abrir archivo
    file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    // Leer header (54 bytes)
    if (fread(header, 1, 54, file) != 54) {
        fclose(file);
        return NULL;
    }

    // Verificar que sea BMP
    if (header[0] != 'B' || header[1] != 'M') {
        fclose(file);
        return NULL;
    }

    // Leer información del header
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    *width = *(int*)&(header[0x12]);
    *height = *(int*)&(header[0x16]);

    // Verificar profundidad de bits (debe ser 24 bits)
    unsigned short bitsPerPixel = *(unsigned short*)&(header[0x1C]);
    if (bitsPerPixel != 24) {
        fclose(file);
        return NULL;
    }

    // Calcular padding (las filas BMP son múltiplos de 4 bytes)
    padding = (4 - ((*width) * 3) % 4) % 4;
    
    // Calcular tamaño real con padding
    int rowSize = (*width) * 3 + padding;
    imageSize = rowSize * (*height);
    
    if (dataPos == 0) dataPos = 54;

    // Crear buffer para la imagen SIN padding
    data = (unsigned char*)malloc((*width) * (*height) * 3);
    if (!data) {
        fclose(file);
        return NULL;
    }

    // Leer datos fila por fila, eliminando el padding
    fseek(file, dataPos, SEEK_SET);
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);
    
    for (row = 0; row < *height; row++) {
        fread(rowBuffer, 1, rowSize, file);
        memcpy(data + row * (*width) * 3, rowBuffer, (*width) * 3);
    }
    
    free(rowBuffer);
    fclose(file);
    return data;
}

void inicializarTexturas() {
    glEnable(GL_TEXTURE_2D);
    texInicioMictlan.data = cargarBMP("inicio_mictlan.bmp", &texInicioMictlan.width, &texInicioMictlan.height);    
    if (texInicioMictlan.data != NULL) {
        glGenTextures(1, &texturaInicioMictlan);
        glBindTexture(GL_TEXTURE_2D, texturaInicioMictlan);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texInicioMictlan.width, texInicioMictlan.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texInicioMictlan.data);
        texturas_cargadas++;
    } 
    texFondoSala.data = cargarBMP("FondoSala.bmp", &texFondoSala.width, &texFondoSala.height);    
    if (texFondoSala.data != NULL) {
        glGenTextures(1, &texturaFondoSala);
        glBindTexture(GL_TEXTURE_2D, texturaFondoSala);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoSala.width, texFondoSala.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoSala.data);
        texturas_cargadas++;
    } 

    texAltar.data = cargarBMP("altar.bmp", &texAltar.width, &texAltar.height); 
    if (texAltar.data != NULL) {
        glGenTextures(1, &texturaAltar);
        glBindTexture(GL_TEXTURE_2D, texturaAltar);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texAltar.width, texAltar.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texAltar.data);
        texturas_cargadas++;
    
    }
    texFondoCuarto.data = cargarBMP("FondoCuarto.bmp", &texFondoCuarto.width, &texFondoCuarto.height);
    if (texFondoCuarto.data != NULL) {
        glGenTextures(1, &texturaFondoCuarto);
        glBindTexture(GL_TEXTURE_2D, texturaFondoCuarto);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoCuarto.width, texFondoCuarto.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoCuarto.data);
        texturas_cargadas++;
    } 

    texFondoGraduacion.data = cargarBMP("FondoGraduacion.bmp", &texFondoGraduacion.width, &texFondoGraduacion.height);
    
    if (texFondoGraduacion.data != NULL) {
        glGenTextures(1, &texturaFondoGraduacion);
        glBindTexture(GL_TEXTURE_2D, texturaFondoGraduacion);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoGraduacion.width, texFondoGraduacion.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoGraduacion.data);
        texturas_cargadas++;
    } 

    texFondoOficina.data = cargarBMP("FondoOficina.bmp", &texFondoOficina.width, &texFondoOficina.height);
    if (texFondoOficina.data != NULL) {
        glGenTextures(1, &texturaFondoOficina);
        glBindTexture(GL_TEXTURE_2D, texturaFondoOficina);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoOficina.width, texFondoOficina.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoOficina.data);
        texturas_cargadas++;
    } 

    texFondoNoche.data = cargarBMP("FondoNoche.bmp", &texFondoNoche.width, &texFondoNoche.height);
    if (texFondoNoche.data != NULL) {
        glGenTextures(1, &texturaFondoNoche);
        glBindTexture(GL_TEXTURE_2D, texturaFondoNoche);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoNoche.width, texFondoNoche.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoNoche.data);
        texturas_cargadas++;
    } 
    texFondoFantasma.data = cargarBMP("FondoFantasma.bmp", &texFondoFantasma.width, &texFondoFantasma.height);
    
    if (texFondoFantasma.data != NULL) {
        glGenTextures(1, &texturaFondoFantasma);
        glBindTexture(GL_TEXTURE_2D, texturaFondoFantasma);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoFantasma.width, texFondoFantasma.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoFantasma.data);
        texturas_cargadas++;
    } 

    texcaminoMictlan.data = cargarBMP("caminoMictlan.bmp", &texcaminoMictlan.width, &texcaminoMictlan.height);
    
    if (texcaminoMictlan.data != NULL) {
        glGenTextures(1, &texturacaminoMictlan);
        glBindTexture(GL_TEXTURE_2D, texturacaminoMictlan);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texcaminoMictlan.width, texcaminoMictlan.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texcaminoMictlan.data);
        texturas_cargadas++;
    } 

     texFondoCenaNavidad.data = cargarBMP("FondoCenaNavidad.bmp", &texFondoCenaNavidad.width, &texFondoCenaNavidad.height);
    
    if (texFondoCenaNavidad.data != NULL) {
        glGenTextures(1, &texturaFondoCenaNavidad);
        glBindTexture(GL_TEXTURE_2D, texturaFondoCenaNavidad);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoCenaNavidad.width, texFondoCenaNavidad.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoCenaNavidad.data);
        texturas_cargadas++;
    } 

    texFondoArbol.data = cargarBMP("FondoArbol.bmp", &texFondoArbol.width, &texFondoArbol.height);
    
    if (texFondoArbol.data != NULL) {
        glGenTextures(1, &texturaFondoArbol);
        glBindTexture(GL_TEXTURE_2D, texturaFondoArbol);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoArbol.width, texFondoArbol.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoArbol.data);
        texturas_cargadas++;
    } 

    texFondoEsc4.data = cargarBMP("FondoEsc4.bmp", &texFondoEsc4.width, &texFondoEsc4.height);
    
    if (texFondoEsc4.data != NULL) {
        glGenTextures(1, &texturaFondoEsc4);
        glBindTexture(GL_TEXTURE_2D, texturaFondoEsc4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texFondoEsc4.width, texFondoEsc4.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texFondoEsc4.data);
        texturas_cargadas++;
    } 

    texfondoEsc7.data = cargarBMP("fondoEsc7.bmp", &texfondoEsc7.width, &texfondoEsc7.height);


    if (texfondoEsc7.data != NULL) {
        glGenTextures(1, &texturafondoEsc7);
        glBindTexture(GL_TEXTURE_2D, texturafondoEsc7);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texfondoEsc7.width, texfondoEsc7.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texfondoEsc7.data);
        texturas_cargadas++;
    } 

    glDisable(GL_TEXTURE_2D);

}

void dibujarRectanguloTexturizado(float x, float y, float ancho, float alto, GLuint textura) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textura);
    
    glColor3f(1.0, 1.0, 1.0);  
    glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0); glVertex2f(x, y);
        glTexCoord2f(1.0, 0.0); glVertex2f(x + ancho, y);
        glTexCoord2f(1.0, 1.0); glVertex2f(x + ancho, y + alto);
        glTexCoord2f(0.0, 1.0); glVertex2f(x, y + alto);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}


