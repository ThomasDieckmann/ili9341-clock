# Uhr mit ILI9341 display

Das war eigentlich eine schnelle Bastelei, die auch zugegebenermaßen noch nicht ganz fertig ist.
So sieht der Code allerdings auch aus. Da könnten irgendwann noch einmal ein paar Kommentare hinein.

Hinweise:
- Viele dieser LCD-Displays kommen als 3,3V-Version ohne Pegelwandler auf den Markt. Daher habe ich
  im Schaltplan die Spannungsteiler eingesetzt. Sollten Pegelwandler verbaut sein, kann der Spannungsteiler
  entfallen und das Display direkt angeschlossen werden.
- Dass alle Segmente gezeichnet werden, ist gewollt. Die inaktiven Segmente werden in einer dunkleren
  Farbe dargestellt, damit der Eindruck eines 7-Segment LED-Displays entsteht. Wen's stört, der kann
  das im Code gerne auf schwarz stellen.

TODO:
- Integration LDR im Code => Steuern LED Hintergrundbeleuchtung
- Alarmfunktion
- Chime-Funktion
- präziseres RTC-Modul finden
