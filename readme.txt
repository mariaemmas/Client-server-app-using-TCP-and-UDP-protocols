Stan Maria-Emma grupa 321CD


    TEMA 2 Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

1. Consideratii initiale
    Realizarea aplicatiei s-a bazat pe cerintele temei de casa.
    In cele ce urmeaza voi prezenta
        - structurile de date alese
        - consideratii privind realizarea serverului
        - consideratii privind realizarea clientului TCP


2. Structuri de date
    Pentru comunicatia intre server si clienti s-a ales varianta mesajelor cu lungime fixa.

2.1 Structuri de date utile in parsarea mesajelor. 
    Nu sunt folosite pentru stocarea datelor.
    Informatiile transmise de la clientii UDP sau intre server si clientii TCP sunt organizate sub forma
acestor structuri. Exista proceduri pentru:
    parsarea mesajelor UDP (parse_udp_msg) care transforma mesajele in structuri UDP_MESSAGE
    parsarea mesajelor TCP (parse_tcp_message) care transforma mesajele in structuri TCP_MESSAGE
    asamblarea structurii TCP_MESSAGE (compose_tcp_message) sub forma de mesaj (sir de caractere)

2.1.1. UDP_MESSAGE are structura impusa in cadrul temei : topic, tip_date, continut. Are lungime fixa (1551chr).
    Este utilizata de catre server pentru a analiza topicul primit de la clientul UDP si a hotari
catre ce clienti TCP trebuie transmis mesajul (sau pentru ce clienti inactivi stocat). 

2.1.2. TCP_MESSAGE reprezinta o structura ce contine informatiile transmise intre server si clientii TCP.
    Joaca rol de structura de protocol de comunicatie. Trebuie sa acopere necesitatile de transmitere
bidirectionala a informatiilor. Are lungime fixa (1580 chr). Contine informatiile din UDP_MESSAGE, adresa IP
si portul clientului UDP, id_ul clientului TCP si indicatorul sf. In plus am adaugat un indicator 
content_type utilizat pentru depanarea aplicatiei dar si pentru a face actiunile mai clare.

2.2 Structuri de date care au rolul de a stoca informatii.
    Sunt folosite de catre server.
    S-a ales varianta de a stabili legaturi intre structuri pe baza de indecsi.

2.2.1. CLIENT_TCP contine informatii despre clientii TCP conectati sau deconectati. 
    Acestea sunt ID-ul clientului, socketul
     pe care s-a conectat, daca este conectat sau nu, numarul
de topicuri subscrise, id-ul topicurilor subscrise si indicatorul sf al acestora.

2.2.2. TOPIC contine topicurile : id si denumire. In varianta in care in clienti nu ar fi memorate 
id-urile ci topicurile aceasta structura ar putea lipsi. Pentru claritatea analizei datelor am 
preferat sa o creez.

2.2.3. MESSAGE_STORED contine mesaje stocate.
    Cand este primit un mesaj de la un client UDP se cauta clientii TCP care au subscris la topicul
inclus in mesaj. Pentru clientii conectati mesajul este transmis. Pentru clientii deconectati 
care au subscris pentru topic cu indicatorul sf mesajul este memorat in aceasta structura.
    Informatiile cuprinse in structura sunt indexul mesajului, mesajul si id_ul clientului.
    Mesajul stocat cuprinde datele din structura TCP_MESSAGE cunoscute : informatiile din 
UDP_MESSAGE, IP-ul si portul clientului UDP.

2.2.4. MEGGAGE_FOR contine indexul de mesaj stocat si id_ul clientului deconectat care are dreptul
sa primeasca mesajul la conectare.


3. Realizarea serverului (server.c)
    Serverul primeste ca si parametru portul ce care se conecteaza.
    Sarcini pe care trebuie sa le indeplineasca:
3.1. pornire server. Presupune crearea socketilor TCP si UDP, inhibarea algoritmului NAGLE, bind
intre socketii TCP si UDP si port, pornirea actiunii de listen pe portul TCP precum si definirea 
unei structuri fd_set care va ajuta serverul sa asculte pe mai multe socketuri simultan 
(standard input, UDP, TCP server si clienti).
3.2. citire de la standard input - poate primi doar comanda exit. In acest caz serverul va trimite
mesaje de deconectare catre clientii TCP si se va inchide. 
3.3. tratare intrare pe socketul server TCP, dupa caz:
    - adaugare socket client nou si citire informatii despre acesta.
    - activare client deazctivat, cu transmitere mesaje stocate daca exista
    - semnalare client already connected daca este cazul
3.4 tratare intrare pe un socket client, dupa caz
    - deconectare client daca numarul de biti primit prin recv este 0
    - in cazul primirii comenzii subscribe memoreaza in dreptul clientului topicul si indicatorul sf
    - in cazul primirii comenzii unsubscribe sterge topicul din dreptul clientului
3.5 tratare intrare pe socketul UDP - mesajul primit este 
    - transmis clientilor care sunt conectati si au subscris
    - memorat pentru clientii deconectati care au subscris la topic cu indicatorul sf
 
4. Realizarea clientului TCP (subscriber.c)
    Clientul TCP primeste ca si parametri id_ul clientului, IP-ul si portul de conectare.
    Sarcini pe care trebuie sa le indeplineasca:
4.1. pornire subscriber. Presupune crearea socketului TCP, inhibarea algoritmului NAGLE, conectarea 
la server si crearea unei structuri fd_set care sa asigure interceptarea evenimentelor de la
standard input si de pe socketul TCP.
4.2. trimitere care server a informatiei : clientul cu id_ul primit ca parametru s-a conectat.
4.3. citire de la standard input, dupa caz
    - in cazul primirii comenzii exit inchide clientul
    - pentru comanda subscribe <topic> <sf> va trimite comanda catre server
    - pentru comanda unsubscribe <topic> va trimite comanda catre server
4.4 citire de la socketul TCP : interpreteaza mesajul primit prin comanda recv si il afiseaza 
conform indicatiilor.



