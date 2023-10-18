# ULP1: MMCP

Ursprüglich für die Synchronisierung von Murmelbahnen vorgesehen, wird dieses Protokoll eingesetzt um verteilte Systeme untereinander kommunizieren zu lassen.
Das MMCP implementiert einige der OSI Layer und berücksichtigt die besondere Topologie der Daisy Chain.
Der Master/PC schickt die Informationen los.
Der erste Teilnehmer empfängt die Daten und schickt sie an den nächsten Teilnehmer, wenn er nicht adressiert wurde.
Ist die Nachricht beim adressierten Teilnehmer eingetroffen, wird die entsprechende Antwort sofort erzeugt und weitergeschickt.
Alle folgenden Teilnehmer reichen sie weiter und der Master empfängt die Antwort.
Kommt das Paket unbeantwortet wieder beim Master an, hat sich kein Teilnehmer angesprochen gefühlt, es war also keiner adressiert.
Grundsätzlich initiiert nur der Master einen Datenaustausch.
Ein adressierter Teilnehmer antwortet, alle anderen leiten die Pakete weiter.
Teilnehmer senden nie ein Paket ab, ohne vorher eine Anforderung vom Master empfangen zu haben.

## Übersicht über die Schichten:

### Anwendungsschicht 
Das erste Byte bestimmt den Nutzdateninhalt, also z.B. ob die Nutzdaten die Farbe/Helligkeit einer LED angeben oder die UID des Boards oder nur das Ereignis zählt (ein Kontrollfluss erzeugt wird), also die Nutzdaten nicht von Interesse sind.
Die folgenden 8 Byte sind für Nutzdaten vorgesehen.
Wie sie gefüllt werden, bestimmt das erste Byte. 
Die ersten Funktionen implementieren Sie bereits hier.

### Vermittlungsschicht
Die Daten, die gesendet werden, bekommen 4 Bytes hinzugefügt: Empfänger-Adresse, Sender-Adresse, Versionsnummer und Hops, ein Zähler, wie oft die Nachricht weitergeleitet wurde.

### Sicherungsschicht
Die übertragenen Daten bekommen ein Byte Prüfsumme angehängt, die durch Addition der Nutzdaten erzeugt wird.
Beim Empfang wird die Prüfsumme ausgerechnet.
Stimmt sie, werden die Daten an die höhere Schicht weitergegeben, ansonsten verworfen.

### Bitübertragungsschicht 1
Die Nutzdaten werden mit jeweils einem Byte eingerahmt, um Fehler beim Start und Ende der Übertragung zu minimieren.
Der Sender sendet eine 0, der Empfänger ignoriert das erste und letzte Byte.

### Bitübertragungsschicht 2
Im Mikrocontroller befindet sich ein USART des Nucleo F401 und hier wird nur der asynchrone Übertragungsmodus genutzt.
Die Protokollmaschine ist Teil der digitalen Logik und wird von der CPU über Register konfiguriert (z.B. die Baudrate von 115.200 bit/s).
Über Interrupts signalisiert die Hardware der CPU unterschiedliche Zustände.
Am häufigsten genutzt wird der Empfang von einem Zeichen und die Signalisierung, dass ein Sendevorgang abgeschlossen wurde, also wieder Platz im Sendepuffer für neue Daten ist.

### Bitübertragungsschicht 3
Auf der Peripherieplatine befindet sich ein Pegelwandler, der den störunempfindlichen RS-232 Pegel mit großem Spannungshub ( +/- 3..15V) auf den Low-Voltage Pegel (0 / 3,3 V) der I/O Pins des Mikrocontrollers wandelt.

Um eine reibungslose Kommunikation zu gewährleisten brauchen Sie eine eindeutige Adresse.
Die können Sie jetzt als globalen uint_8 Wert anlegen (uint8_t myAddress) und ihre auf dem Board vermerkte, handschriftliche Nummer (in meinem Fall 3) dafür einsetzen.
Der USART2 des Nucleo Boards wird über den STLink weitergereicht und auf dem PC als Virtual COM Port registriert.

## MMCP Übersicht

L1 |L3 |L3 |L3 |L3 |L7 |L7 |L7 |L7 |L7 |L7 |L7 |L7 |L7 |L2 |L1
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---
SOF | To | From | Vers | Hops | ApNr | L7_SDU[0] | L7_SDU[1] | L7_SDU[2] | L7_SDU[3] | L7_SDU[4] | L7_SDU[5] | L7_SDU[6] | L7_SDU[7] | Checksum | EOF

Legende | Beschreibung
---|---
L1        | Layer 1, Bitübertragungsschicht
L2        | Sicherungsschicht
L3        | Vermittlungsschicht
L7        | Anwendungsschicht
SOF       |
To        | Empfängeradresse
From      | 0: Masterpaket; sonst: Antwortpaket
Vers      | Versionsnummer, hier: 5
Hops      | Zähler
ApNr      | Applikationsnummer
L7_SDU[i] |
Checksum  |
EOF       |

In den folgenden Aufgaben sollen Sie Felder anlegen.
Zur besseren Lesbarkeit wurde die Größenangabe weggelassen.
Sie sollten die Größen der entsprechenden Felder vorher berechnen oder im Aufgabentext finden und in ihrem Code/Header anlegen.
Für den Layer 7 ist das hier schon vorgegeben.
Weiterhin sind im Text genutzte Variablen/Defines vorgegeben.

```C
/* GLOBALS */
uint8_t myAddress = 0x2a;
/* DEFINES */
#define MMCP_MASTER_ADDRESS 0
#define MMCP_VERSION 4
#define L7_PDU_size 9
#define L7_SDU_size 8
#define L7_PCI_size 1
#define L3_PDU_size ?
#define L3_SDU_size ?
#define L3_PCI_size ?
#define L2_PDU_size ?
#define L2_SDU_size 13
#define L2_PCI_size ?
#define L1_PDU_size ?
#define L1_SDU_size ?
#define L1_PCI_size ?
```

## L1_receive
Die Callbackfunktion `HAL_UART_RxCpltCallback` läuft in der Regel im Kontext des Interrupthandlers des USARTs.
Um in diesem Kontext möglichst kurz zu verweilen, sollte lediglich die L1_PDU gesichert werden und ein Flag vom Typ bool gesetzt werden, das dann in der main-loop ausgewertet wird.
In der main-loop wird dann abhängig vom gesetzten Flag Ihre Funktion `L1_receive(uint8_t L1_PDU[])` aufgerufen.
Dort kopieren Sie nur die relevanten Bytes (ohne das 1. Byte SOF und das letztes Byte EOF, die PCI dieses Layers) in ein neu angelegtes Feld `uint8_t L1_SDU[]` und rufen am Ende dieser Funktion `L2_receive` auf und
übergeben die L1-SDU.

## L2_receive
Diese Funktion bekommt die SDU des L1 als PDU, deshalb ist der Prototyp: `L2_receive(uint8_t L2_PDU[])`.
Berechnen und Prüfen Sie den CRC.
Nur wenn die Prüfung des CRC erfolgreich ist, geben Sie die 13 Byte große L2_SDU weiter an den nächsten Layer.
Stimmt die CRC Summe nicht, ist keine weitere Aktion nötig.
Über einen Timeout kann der Absender erfahren, dass das Paket verlorengegangen ist.
Die CRC Berechung ohne Reflektion inititalisiert das 1-Byte Register mit 0 und nutzt 0x07 als Generatorpolynom.
Konkrete Hinweise zur CRC-Berechnung finden sich auf den hier verlinkten Seiten
von [Michael Barr](https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code).
Eine online Überprüfung Ihres CRC-Codes können Sie hier durchführen.

## L3_receive
Hier wird geprüft, ob ein Paket vom Master gesendet wurde, oder an ihn gesendet wird.
Andere Adressierungsarten sind nicht zugelassen.
Prototyp: `L3_receive(uint8_t L3_PDU[])`.
Kopieren Sie die SDU in ein neues Feld `uint8_t L3_SDU[]`.
Hier entscheidet sich, ob das Paket für Sie bestimmt ist, also an den höheren Layer 7 (4,5 und 6 sind nicht spezifiziert) weitergegeben wird, oder weitergeleitet wird, also über L2_send zurückgegeben wird.
Die PCI dieses Layers hat 4 Protokollfelder:
- To: Die Empfängeradresse. Stimmt diese mit der eigenen Adresse uint8_t myAddress überein und ist der Absender 0 (also der Master), wird die 9 Byte große SDU an den Layer 7 durch Aufruf
von L7_receive weitergegeben. Ist das To-Feld 0, wird die Nachricht immer weitergeleitet (Eine Ausnahme gibt es: Wenn To- und From-Feld jeweils 0 sind, wird das Paket verworfen).
- From: Kommt ein Paket vom Master, so steht hier eine 0. Wenn es ein anderer Wert ist, so handelt es sich um ein Antwortpaket eines anderen Teilnehmers, das wir weiterleiten müssen, sofern die Empfängeradresse 0 ist (es also an den Master geht).
- Vers: Aktuell ist die Version 5. Steht dort eine andere Versionsnummer, muss das Paket verworfen werden.
- Hops: Der Hopcounter zählt, wie oft das Paket weitergeleitet wurde. Er muss um einen hochgezählt werden, bevor weitergeleitet wird, also das Paket an L2_send übergeben wird.

## L7_receive
Prototyp: `L7_receive(uint8_t L7_PDU[])`.
Isolieren Sie auch hier die 8 Byte große SDU: `uint8_t L7_SDU[]`.
Das weitere Verhalten hängt nun von der Applikationsnummer ApNr ab.
Zwei Grundsätzliche Regeln müssen jedoch unabhängig von der ApNr eingehalten werden:
1. Jedes Paket wird mit einem Paket beantwortet; nach dem Motto: Eins rein, eins raus. Keine Antwort bedeutet einen Fehler, wie z.B. der Erhalt einer nicht-implementierten ApNr. Mehr als ein Paket als Antwort ist nicht erlaubt. Dies würde zu Engpässen bei den danach kommenden Teilnehmern führen, die die Pakete weiterleiten müssen.
2. Die Antwort muss so schnell wie möglich abgesendet werden. Es ist Zeit eine Variable oder einen Port zu lesen und zurück zu gegeben. Verzögerungen an dieser Stelle führen ebenso zu Engpässen. Erzeugen Sie keine Textausgaben (die brauchen verhältnismäßig viel Zeit) während der Beantwortung.

Daraus folgt, dass Sie hier direkt die Antwortfunktion L7_send aufrufen.
Alle zeitintensiven Bearbeitungen, die durch den Empfang eines Pakets ausgelöst werden, sollen durch das Setzen eines Flags in das Hauptprogramm ausgelagert werden.
Dort können Sie das Flag testen und nach Bearbeitung zurücksetzen.
Das weitere Verhalten hängt nun von der ID ab.
Die Weiterleitung des Pakets an den nächsten Teilnehmern wird von Layer 3 übernommen.
Implementieren Sie folgende IDs:

### ID 100: LED an/aus
Wenn das letze Byte (L7_SDU[7]) ungleich 0 ist, wird die onboard LED angestellt, ist es gleich 0, wird sie ausgeschaltet.

ApNr 100: | L7_SDU[0] | L7_SDU[1] | L7_SDU[2] | L7_SDU[3] | L7_SDU[4] | L7_SDU[5] | L7_SDU[6] | L7_SDU[7]
---|---|---|---|---|---|---|---|---
Inhalt: | - | - | - | - | - | - | - | 0 / nicht 0

Als Antwort wird eine Kopie der Anfrage versendet.

### ID 101: Anzahl Tastenbetätigungen einlesen
Zählen Sie (in der Callbackroutine des EXTI Interrupts), wie oft der Taster gedrückt wurde.
Setzen Sie das Entprellen mit einem HAL_delay oder eleganter Timer um!
Wenn sie vom Master über diese ApNr die Anfrage für diesen Wert erhalten, füllen Sie im Antwortpaket das letzte Byte mit der Anzahl der Tastenbetätigungen und setzen danach die Zählvariable auf 0 zurück, damit bei der nächsten Anfrage die Anzahl der Betätigungen seit dem Auslesen genannt werden.

ApNr 101: | L7_SDU[0] | L7_SDU[1] | L7_SDU[2] | L7_SDU[3] | L7_SDU[4] | L7_SDU[5] | L7_SDU[6] | L7_SDU[7]
---|---|---|---|---|---|---|---|---
Inhalt: | - | - | - | - | - | - | - | Zähler

### ApNr 102 und 103: UID auslesen und zurückgeben
Jeder ST Mikrocontroller besitzt eine eindeutige, einzigartige Nummer (Unique Identifier).
Bevorzugt sollten Sie die HAL System Driver Funktionen nutzen:
- first word of UID (Bit 31..0) : `uint32_t HAL_GetUIDw0 (void )`
- second word of UID (Bit 63..32): `uint32_t HAL_GetUIDw1 (void )`
- third word of UID: (Bit 95..64): `uint32_t HAL_GetUIDw2 (void )`

Alternativ können Sie auch die auf [techoverflow.net](https://techoverflow.net/) gezeigte Lösung nutzen.

Der Inhalt der ankommenden SDU von ApNr 102 wird ignoriert.
Formatieren Sie die Rückgabe-SDU nach folgender Aufteilung:

ApNr 102: | L7_SDU[0] | L7_SDU[1] | L7_SDU[2] | L7_SDU[3] | L7_SDU[4] | L7_SDU[5] | L7_SDU[6] | L7_SDU[7]
---|---|---|---|---|---|---|---|---
UID Bit: | 7..0 | 15..8 | 23..16 | 31..24 | 39..32 | 47..40 | 55..48 | 63..56

Der Inhalt der ankommenden SDU von ApNr 103 wird ignoriert.
Formatieren Sie die Rückgabe-SDU nach folgender Aufteilung:

ApNr 103: | L7_SDU[0] | L7_SDU[1] | L7_SDU[2] | L7_SDU[3] | L7_SDU[4] | L7_SDU[5] | L7_SDU[6] | L7_SDU[7]
---|---|---|---|---|---|---|---|---
UID Bit: | 71..64 | 79..72 | 87..80 | 95..88 | - | - | - | -

Beispiel für die UID 0xbbaa99887766554433221100 (Bits 95..64 = 0xbb, also binär 10111011):

HAL_GetUIDw0 gibt zurück: 0x33221100

HAL_GetUIDw1 gibt zurück: 0x77665544

HAL_GetUIDw2 gibt zurück: 0xbbaa9988

Die SDU von ApNr 102 enthält u.a.: L7_SDU[0] = 0x00 , L7_SDU[7] = 0x77

Die SDU von ApNr 103 enthält u.a.: L7_SDU[0] = 0x88 , L7_SDU[3] = 0xbb

Diese Abbildung ist durch ein einfaches casten möglich (auf little-endian Systemen, wie es das genutzte Board ist).
Die erzeugte Layer 7 SDU wird zusammen mit der Applikationsnummer ApNr an L7_send übergeben.

## L7_send
Prototyp: `L7_send(uint8_t Id, uint8_t L7_SDU[])`.
Setzen Sie Id und SDU zur Layer 7 PDU (`uint8_t L7_PDU[]`) zusammen.
Die übergeben Sie als Layer 3 SDU beim Aufruf an L3_send.

## L3_send
Prototyp: `L3_send(uint8_t L3_SDU[])`.
Die Layer 3 PCI enthält als Adressat (To-Feld) immer die Adresse 0, da nur an den Master zurückgesendet wird, die Absenderadresse uint8_t myAddress, Version 5, und einen auf 0 initialisierten Hopcounter.
Kombinieren Sie die 4 Byte PCI mit der SDU zur Layer 3 PDU (`uint8_t L3_PDU[]`).
Die übergeben Sie als Layer 2 SDU beim Aufruf an L2_send.

## L2_send
Prototyp: `L2_send(uint8_t L2_SDU[])`.
Die Layer 2 PCI enthält die Prüfsumme.
Berechnen Sie sie über die SDU und kombinieren SDU und PCI zur `uint8_t L2_PDU[]`, die an L1_send weitergegeben wird.

## L1_send
Prototyp: `L1_send(uint8_t L1_SDU[])`.
Die Layer 1 PCI besteht aus den beiden Feldern SOF und EOF, die Sie mit 0 füllen.
Kombinieren Sie SDU und PCI zur 16 Byte großen `uint8_t L1_PDU[]`.
Versenden Sie sie über den USART mit dem Aufruf `HAL_UART_Transmit()` im Blocking Mode oder nehmen Sie die nicht blockierende Version (Anhängsel `_IT()`, um den Programmablauf nicht für die komplette Zeit des Versendens zu unterbrechen (Non Blocking Mode).
Die Dauer des Versendens beträgt: 16byte * 10bit/byte * 1/(115200sec/bit) ≈ 1,4ms.
Der Prozessor mit 84 MHz könnte in dieser Zeit 50.000 bis 100.000 Assemblerinstruktionen ausführen.
