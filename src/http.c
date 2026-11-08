/*
Modulo de parseo de una peticion HTTP 
este modulo tomara un arreglo de bytes y separa
los encabezados, comandos, y contenido a la vez que verifica la sintaxis http 
y la disponibilidad de recursos para responder con los codigos de respuesta
correspondiente a la vez que arma los encabezados
*/

#include<string.h>
#include<stdio.h>
#include<error.h>

struct http_header_ {
    char encabezado[200];
    int idx_encabezado;
    char value[300];
    int idx_value;
};
struct first_line {
    char method[15];
    int idx_method;
    char path[120];
    int idx_path;
};
struct http_msg_ { 
    char *buffer_read;
    int max_len_buffer;
    int idx_begin;
    int idx_end;
    int len_read;
    struct first_line f_line;    
    struct http_header_ headers[30];
    int max_len_header;
    int idx_header;
};


char peticion_ejemplo[]=""
"GET /index.html HTTP/1.1 \r\n"
"Host: www.ejemplo.com \r\n"
"User-Agent: MiClienteHTTP/1.0\r\n"
"Accept: text/html \r\n"
"Accept-Encoding: gzip, deflate\r\n"
"Connection: keep-alive"
" content-lenght=1000"
"\r\n\r\n"
"GET /index.html HTTP/1.1 \r\n"
"Host: www.ejemplo.com \r\n"
"User-Agent: MiClienteHTTP/1.0\r\n"
"Accept: text/html \r\n"
"Accept-Encoding: gzip, deflate\r\n"
"Connection: keep-alive"
"\r\n\r\n"
"GET /index.html HTTP/1.1 \r\n"
"Host: www.ejemplo.com \r\n";

void parserHttp(struct http_msg_ *msg){
    //Buffer circular 
    for(int idx=msg->idx_begin;idx+3<msg->len_read;idx++){
        if (msg->buffer_read[idx%msg->max_len_buffer]     == '\r' && msg->buffer_read[(idx+ 1)%msg->max_len_buffer ] == '\n' && msg->buffer_read[(idx+ 2)%msg->max_len_buffer ] == '\r' && msg->buffer_read[(idx+ 3)%msg->max_len_buffer ] == '\n') {
            msg->idx_end=idx+3;
            afd_parsing(msg);
            break;
        }
    }
    msg->idx_end=-1;
}
void parsing_first_line(struct http_msg_ *msg){
    int i;
    for(i=0;msg->buffer_read[(msg->idx_begin+i)%msg->max_len_buffer]!=' ' && i<7;i++){ // el len maximo de un metodo estandar de http es 7 options
        msg->f_line.method[i]=msg->buffer_read[(msg->idx_begin+i)%msg->max_len_buffer];
    }
}
/*
// Matriz de transicion
simbolos encabezado se = [-_#$%^&'*+-~=]

    [a-Z]   [0-9]    ' '    /t    /n      /r     /  se  :  /0        h    t   p   .
q0    1               1     1                                          1   1   1      // iniciar indice metodo
q1    1               2     2                                           1   1   1       // terminar indice metodo si transicion es /t o ' '                              
q2                    2     2                   3                                       //iniciar indice path si transion es /                                                                            
q3    3        3      4     4                   3   3   3               3   3   3   3   // terminar indica path si transicion es /t ' '
q4                    4     4                                           5                              
q5                                                                          6
q6                                                                          7
q7                                                                              8
q8                                              9
q9              9                                                                   10
q10             10                          11
q11                                 12      
q12   13       13      12     12            17      13                  13  13  13  13   // iniciar indice encabezado si transicion es hacia 13
q13   13        13                                  13   14             13  13  13  13   // terminar indice encabezado si transicion es 14
q14   15        15     14    14                  15  15  15             15  15  15  15  // iniciar indice valor si transicion es 14 terminar si es 16
q15   15        15     15    15           16     15  15  15             15  15  15  15
q16                                 17       
q17   13       13      12     12          18
q18                                 19
q19 aceptacion
*/
void afd_parsing(struct http_msg_ *msg){
    /*Vocabulario*/
    int matrizTransicion[20][15]={
        // se= [! # $ % & ' * + - . ^ _] //33 -39 42-45  94  95 96
        //  0       1       2      3     4      5        6   7   8  9        10     11  12   13    14
        //[a-Z]   [0-9]    ' '    /t    /n      /r       //  se  :  /0        h     t    p    .     resto
        {   1,     -1,     -1,    -1,   -1,     -1,     -1, -1, -1, -1,       1,    1,  1,  -1 ,    -9},//q0 
        {   1,     -1,      2,     2,   -1,     -1,     -1, -1, -1, -1,       1,    1,  1,  -1 ,    -9},//q1  // -1 metodo incorrecto sintaxis
        {  -2,     -2,      2,     2,   -2,     -2,      3, -2, -2, -2,      -2,   -2, -2,  -2 ,    -9},//q2  // -2 separacion de ruta incorrecta
        {   3,      3,      4,     4,   -3,     -3,      3,  3,  3, -3,       3,    3,  3,   3 ,    -9},//q3  // -3 separacion de ruta incorrecta y protocolo
        {  -4,     -4,      4,     4,   -4,     -4,     -4, -4, -4, -4,       5,   -4, -4,  -4 ,    -9},//q4 
        {  -4,     -4,     -4,    -4,   -4,     -4,     -4, -4, -4, -4,      -4,    6, -4,  -4 ,    -9},//q5 
        {  -4,     -4,     -4,    -4,   -4,     -4,     -4, -4, -4, -4,      -4,    7, -4,  -4 ,    -9},//q6 
        {  -4,     -4,     -4,    -4,   -4,     -4,     -4, -4, -4, -4,      -4,   -4,  8,  -4 ,    -9},//q7 
        {  -4,     -4,     -4,    -4,   -4,     -4,      9, -4, -4, -4,      -4,   -4, -4,  -4 ,    -9},//q8 
        {  -4,      9,     -4,    -4,   -4,     -4,     -4, -4, -4, -4,      -4,   -4, -4,  10 ,    -9},//q9 
        {  -4,     10,     -4,    -4,   -4,     11,     -4, -4, -4, -4,      -4,   -4, -4,  -4 ,    -9},//q10 // -4 error protocolo
        {  -5,     -5,     -5,    -5,   12,     -5,     -5, -5, -5, -5,      -5,   -5, -5,  -5 ,    -9},//q11 // -5 delimitacion
        {  13,     13,     -6,    -6,   -6,     -6,     -6, 13, -6, -6,      13,   13, 13,  13 ,    -9},//q12 // -6 error de encabezado
        {  13,     13,     -6,    -6,   -6,     -6,     -6, 13, 14, -6,      13,   13, 13,  13 ,    -9},//q13 // 
        {  15,     15,     14,    14,   -7,     16,     15, 15, 15, -7,      15,   15, 15,  15 ,    -9},//q14 // -7 error de valor
        {  15,     15,     15,    15,   -7,     16,     15, 15, 15, -7,      15,   15, 15,  15 ,    -9},//q15 //
        {  -5,     -5,     -5,    -5,   17,     -5,     -5, -5, -5, -5,      -5,   -5, -5,  -5 ,    -9},//q16 //
        {  13,     13,     -6,    -6,   -6,     18,     -6, 13, -6, -6,      13,   13, 13,  13 ,    -9},//q17 //
        {  -8,     -8,     -8,    -8,   19,     -8,     -8, -8, -8, -8,      -8,   -8, -8,  -8 ,    -9},//q18 // -8 error de terminacion de encabezado  // 19 estado de aceptacion
        {  -8,     -8,     -8,    -8,   -8,     -8,     -8, -8, -8, -8,      -8,   -8, -8,  -8 ,    -9},//q19 // 19 estado de aceptacion
    };
    int filas=0;
    int idx=msg->idx_begin;
    int i=0;
    while(filas>=0 && i<(msg->idx_end-msg->idx_begin)){
        idx=(msg->idx_begin+i)%msg->max_len_buffer;
        if (filas<0){
            //error
            return;
        }else if(filas==19){
            //aceptacion
            return;
        }
        if(msg->buffer_read[idx]==72 || msg->buffer_read[idx]==104){ //valor H y h en ascci
            if((filas==0|| filas==1) && msg->f_line.idx_method<sizeof(msg->f_line.method)){
                msg->f_line.method[msg->f_line.idx_method]=msg->buffer_read[idx];
                msg->f_line.idx_method++;
            }else if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12 || filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][10];
        }else if(msg->buffer_read[idx]==84 || msg->buffer_read[idx]==116){ //valor t y t en ascci
            if((filas==0|| filas==1) && msg->f_line.idx_method<sizeof(msg->f_line.method)){
                msg->f_line.method[idx]=msg->buffer_read[idx];
                msg->f_line.idx_method++;
            }else if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12 || filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][11];
        }
        else if(msg->buffer_read[idx]==80 || msg->buffer_read[idx]==112){ //valor p y P en ascci
            if((filas==0|| filas==1) && msg->f_line.idx_method<sizeof(msg->f_line.method)){
                msg->f_line.method[msg->f_line.idx_method]=msg->buffer_read[idx];
                msg->f_line.idx_method++;
            }else if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12 || filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][12];
        }
        else if((65<=msg->buffer_read[idx] && msg->buffer_read[idx]<=90) || (97<=msg->buffer_read[idx] && msg->buffer_read[idx]<=122)){  // a-Z
            if((filas==0|| filas==1) && msg->f_line.idx_method<sizeof(msg->f_line.method)){
                msg->f_line.method[msg->f_line.idx_method]=msg->buffer_read[idx];
                msg->f_line.idx_method++;
            }else if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12 || filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][0];
        }
        else if((48<=msg->buffer_read[idx] && msg->buffer_read[idx]<=57)){  // 0-9
            if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12 || filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }
            filas=matrizTransicion[filas][1];
        }
        else if(msg->buffer_read[idx]==' '){
            if((filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][2];
        }
        else if(msg->buffer_read[idx]=='\t'){
            if((filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }
            else if((filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }
            filas=matrizTransicion[filas][3];
        }
        else if(msg->buffer_read[idx]=='\n'){
            filas=matrizTransicion[filas][4];
        }
        else if(msg->buffer_read[idx]=='\r'){
            filas=matrizTransicion[filas][5];
        }
        else if(msg->buffer_read[idx]=='/'){
            if((filas==3 || filas==2) && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }
            filas=matrizTransicion[filas][6];
        }
        else if((33<=msg->buffer_read[idx] && msg->buffer_read[idx]<=39)|| (42<=msg->buffer_read[idx] && msg->buffer_read[idx]<=45) || (94<=msg->buffer_read[idx] && msg->buffer_read[idx]<=96)){ // se
            if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12||filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][7];
        }
        else if(msg->buffer_read[idx]==':'){
            if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }
            filas=matrizTransicion[filas][8];
        }
        else if(msg->buffer_read[idx]==NULL){
            filas=matrizTransicion[filas][9];
        }
        else if(msg->buffer_read[idx]=='.'){
            if(filas==3 && msg->f_line.idx_path<sizeof(msg->f_line.path) ){
                msg->f_line.path[msg->f_line.idx_path]=msg->buffer_read[idx];
                msg->f_line.idx_path++;
            }else if((filas==12||filas ==13)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_encabezado<sizeof(msg->headers[msg->idx_header].encabezado)){
                msg->headers[msg->idx_header].encabezado[msg->headers[msg->idx_header].idx_encabezado]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_encabezado++;
            }else if((filas==14 || filas ==15)&& msg->idx_header<msg->max_len_header && msg->headers[msg->idx_header].idx_value<sizeof(msg->headers[msg->idx_header].value)){
                msg->headers[msg->idx_header].value[msg->headers[msg->idx_header].idx_value]=msg->buffer_read[idx];
                msg->headers[msg->idx_header].idx_value++;
            }else if(filas==17 && msg->idx_header<msg->max_len_header){
                msg->idx_header++;
            }
            filas=matrizTransicion[filas][13];
        }else{
            filas=matrizTransicion[filas][14];
        }
        i++;
    }
}

