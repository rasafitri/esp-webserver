const char htmlPage[] = R"=====(
<!DOCTYPE html>
<html lang="de">

<head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8">
  <title>Summer School 2023</title>
  <style>
    :root {
      --primary-color: #F64C72;
      --secondary-color: #2F2FA2;
      --accent-color: #9797D1;
      --text-color: #2F2FA2;
      --background-color: #ffffff;
    }

    body {
      background-color: var(--background-color);
      color: var(--text-color);
      font-family: "National Regular";
      margin: 0;
      padding: 20px;
    }

    h1 {
      color: var(--text-color);
      font-size: 24px;
      margin-bottom: 10px;
      font-weight: bold;
    }

    p {
      font-size: 16px;
      margin-bottom: 20px;
      font-weight: 400;
    }

    #dropArea {
      max-width: 640px;
      height: 320px;
      border: 2px dashed #aaa;
      padding: 20px;
      cursor: pointer;
      text-align: center;
      margin-bottom: 5px;
    }

    .container {
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      height: 320px;
    }

    .centered-paragraph {
      text-align: center;
      font-size: 24pt;
    }

    .image-wrapper {
      position: relative;
    }

    .delete-button {
      position: absolute;
      top: 5px;
      right: 5px;
      background-color: #ff0000;
      color: #fff;
      border: none;
      border-radius: 50%;
      width: 20px;
      height: 20px;
      line-height: 1;
      font-size: 14px;
      cursor: pointer;
    }

    .delete-button:hover {
      background-color: #cc0000;
    }

    #transitionTime {
      width: 100px;
      max-width: 100px;
      padding: 5px;
      font-size: 16px;
      font-weight: 400;
      margin: 10px;
      margin-right: 5px;
    }

    #imgSendButton {
      margin-left: 110px;
    }

    #imgSendButton,
    #txtSendButton {
      background-color: var(--secondary-color);
      color: var(--background-color);
      border: none;
      padding: 10px 20px;
      font-size: 16px;
      cursor: pointer;
    }

    #imgSendButton:hover,
    #txtSendButton:hover {
      background-color: var(--accent-color);
    }

    #imgSendButton:disabled,
    #txtSendButton:disabled {
      cursor: not-allowed;
      background-color: #aaa;
    }

    #textToSend {
      width: 650px;
      max-width: 640px;
      padding: 5px;
      font-size: 16px;
      font-weight: 400;
      margin-right: 10px;
      margin-bottom: 10px;
    }

    #imgInput {
      margin-right: 310px;
    }

    #selColor {
      width: 100px;
      padding: 5px;
      margin-bottom: 10px;
      margin-left: 5px;
    }

    #textMode {
      width: 100px;
      padding: 5px;
      margin-right: 5px;
      margin-left: 22px;
    }

    .info-icon {
      display: inline-block;
      width: 10px;
      height: 10px;
      color: grey;
      border: 2px solid grey;
      border-radius: 50%;
      text-align: center;
      line-height: 10px;
      margin-right: 180px;
      cursor: pointer;
      font-size: 10px;
    }

    .info-tooltip {
      display: none;
      background-color: #f9f9f9;
      color: #333;
      padding: 5px;
      position: absolute;
      z-index: 1;
    }

    .info-icon:hover+.info-tooltip {
      display: block;
    }
  </style>
</head>

<body onload="onPageLoad()">
  <h1>Summer School 2023 - Prototyping für ein gesundes Berlin</h1>
  <p>Hier können Sie ein Bild hochladen, das auf der HUB75 Anzeigetafel angezeigt werden soll</p>
  <form onreset="prepareSendImg()">
    <div id="dropArea">
      <div class="container">
        <p class="centered-paragraph">Bilder hierher ziehen oder hier klicken...</p>
      </div>
    </div>
    <input type="file" id="imgInput" accept="image/*" multiple style="display: none;">

    <label for="transitionTime">Frame Delay [ms]:</label>
    <input type="number" id="transitionTime" name="transitionTime" text="" min="200" max="2000" pattern="[0-9]*" disabled>
    <span class="info-icon">&#63;</span>
    <span class="info-tooltip">
      Die Zeit zwischen den Frames in Millisekunden ohne Trennzeichen<br>
      Gültiger Bereich 200...2000 ms
    </span>
    <button type="reset" id="imgSendButton" disabled>Hochladen</button>
  </form>

  <p>oder Sie können auch nur einen Text senden.</p>
  <form onreset="prepareSendTxt()">
    <div>
      <input type="text" id="textToSend" placeholder="anzuzeigender Text" text="Text, der angezeigt werden soll">
    </div>
    <div>
      <label for="selColor">Wählen Sie eine Farbe für den Text aus</label>
      <input type="color" id="selColor" name="selColor" value="#ff0000">
    </div>
    <label for="textMode">Wie soll der Text angezeigt werden?</label>
    <select name="textMode" id="textMode">
      <option value=" "> </option>
      <option value="static">Static Text</option>
      <option value="scroll">Scroll Text</option>
    </select>
    <span class="info-icon">&#63;</span>
    <span class="info-tooltip">
      Wenn Sie Static Text auswählen, wird der Text komplett angezeigt und automatisch umgebrochen.<br>
      Wenn Sie Scroll Text auswählen, wird der Text als ein Lauftext angezeigt und nicht umgebrochen.<br>
      Umlaute und ß können nicht dargestellt werden.
    </span>
    <button type="reset" id="txtSendButton" disabled>Hochladen</button>
  </form>

  <script src="https://cdn.jsdelivr.net/npm/omggif@1.0.10/omggif.min.js"></script>
  <script>
    //#region Globale Hilfsvariablen
    const maxImages = 3;   // Max 3 Bildern hochladen

    var displayWidth = 0;  // Breite der Anzeige von Server
    var displayHeight = 0; // Höhe der Anzeige von Server

    var isValidImage = false; // Flag Bild valid
    var isValidDelay = true; // Flag Frame Delay valid
    var isValidText = false;  // Flag Text valid
    var isValidColor = true;  // Flag Farbe valid
    var isValidMode = false;  // Flag Textmodus valid

    var uploadedImages = []; // Array der eingegebenen Bilder
    //#endregion

    function onPageLoad() {
      // Beim Laden der Seite wird eine GET SIZE Request gesendet
      // der Server schickt dann als Response die Größe der LED-Anzeige zurück
      // Bei einem Fehler wird der Nutzer benachrichtigt
      fetch('./size')
        .then(response => response.json())
        .then(data => {
          displayWidth = data.size[0];
          displayHeight = data.size[1];
          console.log("Displaygröße ist: " + displayWidth + "x" + displayHeight);
        })
        .catch(error => {
          console.error('Error:', error);
          displayWidth = 64;
          displayHeight = 32;
          alert("Es ist ein Fehler passiert! Bitte laden Sie die Seite neu!")
        });
    }

    //#region Element transitionTime
    var transitionTimeField = document.getElementById("transitionTime");
    const minDelay = parseInt(transitionTimeField.getAttribute('min'));
    const maxDelay = parseInt(transitionTimeField.getAttribute('max'));
    transitionTimeField.addEventListener("input", (e) => {
      // bei jeder Eingabe wird die Zahl geprüft, ob diese im Wertebereich liegt
      const frameDelay = transitionTimeField.value;

      if (isNaN(frameDelay) || frameDelay > maxDelay || frameDelay < minDelay) {
        transitionTimeField.setCustomValidity("Bitte geben Sie Ganzzahlen zwischen " + minDelay + " und " + maxDelay + " ein!");
        transitionTimeField.reportValidity();
        preventDefaults(e);
        isValidDelay = false;
      } else if (!/^[\d]*$/.test(frameDelay)) {
        transitionTimeField.setCustomValidity("Bitte geben Sie nur Ganzzahlen ein!");
        transitionTimeField.reportValidity();
        preventDefaults(e);
        isValidDelay = false;
      } else {
        transitionTimeField.setCustomValidity("");
        isValidDelay = true;
      }
      validityChanged(); // Button Stil ändern
    });
    //#endregion

    //#region Element textToSend
    var textSendField = document.getElementById("textToSend");
    textSendField.addEventListener("input", () => {
      // bei jeder Texteingabe wird die Eingabe ohne führende Leerzeichen
      // und ohne Leerzeichen am Ende geprüft
      const textToSend = textSendField.value.trim();
      if (textToSend.length > 0)
        isValidText = true; // gültiger Text
      else
        isValidText = false; // ungültiger Text
      validityChanged(); // Button Stil ändern
    });
    //#endregion

    //#region Element selColor
    var colorPicker = document.getElementById("selColor");
    colorPicker.addEventListener("change", () => {
      // bei einer Änderung der ausgewählten Farbe wird die ausgewählte Farbe
      // in RGB als eine Variable gespeichert und geprüft, schwarz ist unzulässig
      // Anzeige schwarz (0,0,0) = LED aus
      const rgbValue = hexToRGB(colorPicker.value);
      if (rgbValue[0] == 0 && rgbValue[1] == 0 && rgbValue[2] == 0)
        isValidColor = false; // schwarz, Farbe ungültig
      else
        isValidColor = true; // alle anderen Farben sind ok
      validityChanged(); // Button Stil ändern
    });
    //#endregion

    //#region Element textMode
    var textModeField = document.getElementById("textMode");
    textModeField.addEventListener("change", function () {
      // Bei der Änderung der ausgewählten Textmodus wird die Eingabe geprüft
      if (textModeField.value == " ")
        isValidMode = false; // Leerauswahl ist keine gültige Eingabe
      else
        isValidMode = true; // gültige Textmodus
      validityChanged(); // Button Stil ändern
    });
    //#endregion

    //#region Element dropArea
    var dropArea = document.getElementById('dropArea');

    ['dragenter', 'dragover', 'dragleave', 'drop', 'click'].forEach(e => {
      // preventDefaults dropArea bei allen Aktionen
      dropArea.addEventListener(e, preventDefaults, false);
    });

    ['dragenter', 'dragover'].forEach(e => {
      // dropArea highlight bei Aktion dragenter oder dragover
      dropArea.addEventListener(e, highlight, false);
    });

    ['dragleave', 'drop'].forEach(e => {
      // dropArea unhighlight bei Aktion dragleave oder drop
      dropArea.addEventListener(e, unhighlight, false);
    });

    // handle Eingabe bei Aktion drop
    dropArea.addEventListener('drop', (e) => {
      // bei einer Bildeingabe werden die Bilder geladen und vorbereitet
      preventDefaults(e);
      loadImage(e.dataTransfer.files);
    });

    // handle Eingabe bei Aktion click
    // als hätte man den Button Eingabe geklickt
    dropArea.addEventListener('click', () => imgInput.click());

    function highlight() {
      dropArea.classList.add('highlight');
    }

    function unhighlight() {
      dropArea.classList.remove('highlight');
    }
    //#endregion

    //#region Element imgInput
    var imgInput = document.getElementById('imgInput');
    imgInput.addEventListener("change", (e) => {
      // bei einer Bildeingabe werden die Bilder geladen und vorbereitet
      preventDefaults(e);
      loadImage(e.target.files);
    });
    //#endregion

    //#region Element imgSendButton
    var imgSendButton = document.getElementById("imgSendButton");
    function prepareSendImg() {
      // Beim Klick des Buttons "Bild Hochladen" werden die Bilder skaliert und in ein C Code Array umgewandelt, damit sie auf der LED-Matrixanzeige angezeigt werden können

      if (uploadedImages.length > 1) {
        // es gibt mehrere Bilder, das C Code Array von den Bildern mit der Größe und Delay in JSON Format wird als HTTP POST Request an API endpoint /movingimages gesendet
        // Bei einer Response wird diese als Meldung angezeigt, danach wird die Seite zurückgesetzt
        processImages()
          .then((imagesWithDelay) => {
            console.log(JSON.stringify(imagesWithDelay));
            fetch('./movingimages', {
              method: 'POST',
              headers: {
                "Content-Type": "application/json"
              },
              body: JSON.stringify(imagesWithDelay)
            })
              .then(response => response.text())
              .then(data => {
                alert(data);
                resetImages();
              })
              .catch(error => {
                alert('Error: ', error);
                resetImages();
              });
          });
      }
      else {
        if (uploadedImages[0].type === 'image/gif') {
          // es gibt nur ein Bild als .gif Format, das C Code Array vom Bild mit der Größe in JSON Format wird als HTTP POST Request an API endpoint /gif gesendet
          // Bei einer Response wird diese als Meldung angezeigt, danach wird die Seite zurückgesetzt
          processGif()
            .then(framesWithDelay => {
              console.log(JSON.stringify(framesWithDelay));
              fetch('./gif', {
                method: 'POST',
                headers: {
                  "Content-Type": "application/json"
                },
                body: JSON.stringify(framesWithDelay)
              })
                .then(response => response.text())
                .then(data => {
                  alert(data);
                  resetImages();
                })
                .catch(error => {
                  alert('Error: ', error);
                  resetImages();
                });
            })
            .catch(error => {
              alert('Error: ', error);
            })
        }
        else {
          // es gibt nur ein Bild, das C Code Array vom Bild mit der Größe in JSON Format wird als HTTP POST Request an API endpoint /image gesendet
          // Bei einer Response wird diese als Meldung angezeigt, danach wird die Seite zurückgesetzt
          processImg(uploadedImages[0])
            .then(cCodeWithSize => {
              console.log(JSON.stringify(cCodeWithSize));
              fetch('./image', {
                method: 'POST',
                headers: {
                  "Content-Type": "application/json"
                },
                body: JSON.stringify(cCodeWithSize)
              })
                .then(response => response.text())
                .then(data => {
                  alert(data);
                  resetImages();
                })
                .catch(error => {
                  alert('Error: ', error);
                  resetImages();
                });
            })
            .catch(error => {
              alert('Error: ', error);
            })
        }
      }
    }
    //#endregion

    //#region Element txtSendButton
    var txtSendButton = document.getElementById("txtSendButton");
    function prepareSendTxt() {
      // Beim Klick des Buttons "Text Hochladen" wird der eingegebene Text
      // mit der kompletten Einstellung in JSON Format als HTTP POST Request gesendet
      // Bei einer Response wird diese als Meldung angezeigt, danach wird die Seite zurückgesetzt
      const textSetup = {
        value: textSendField.value,
        color: hexToRGB(colorPicker.value),
        mode: textModeField.value
      };

      console.log(JSON.stringify(textSetup));

      fetch('./text', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json; charset=utf-8'
        },
        body: JSON.stringify(textSetup)
      })
        .then(response => response.text())
        .then(data => {
          alert(data);
        })
        .catch(error => {
          alert('Error:', error);
        });

      isValidText = false;
      isValidMode = false;
      validityChanged();
    }
    //#endregion

    //#region helfende Funktionen
    function loadImage(files) {
      // Bei einer Bildeingabe mit mehr als x Bildern ist es nicht zulässig
      if (uploadedImages.length + files.length > maxImages) {
        alert("Es sind max. nur " + maxImages + " Bilder zulässig!");
      }
      else {
        // jede ausgewählte Datei einzeln anschauen
        for (let i = 0; i < files.length; i++) {
          if (!files[i].type.startsWith('image/')) {
            // Es ist keine Bilddatei, Fehlermeldung anzeigen
            alert("Falsche Datei! Bitte nur Dateien in Bildformat eingeben!");
          }
          else {
            if (files[i].type === 'image/gif') {
              // eine .GIF Datei darf nur einzeln hochgeladen werden
              if (uploadedImages.length > 0) {
                alert("Eine .GIF Datei kann nur einzeln hochgeladen werden!");
                continue;
              }
              // das erste Bild, was hochgeladen wird
              // .gif speichern
              uploadedImages.push(files[i]);
              // dropArea ohne Text für die Bildanzeige
              // bereit zum Senden
              dropArea.innerHTML = '';
              isValidImage = true;
              validityChanged();
              // kein Frame Delay, weil dieser von der Datei übernommen wird
            }
            else {
              // überprüfe ob es schon mal eine .GIF Datei gibt
              if (uploadedImages.length > 0 && uploadedImages[0].type === 'image/gif') {
                alert("Eine .GIF Datei kann nur einzeln hochgeladen werden!");
                continue;
              }

              // keine .GIF Datei, mehrere Bilder ermöglichen
              // in Array speichern und anzeigen
              uploadedImages.push(files[i]);

              if (uploadedImages.length == 1) {
                // das erste Bild, was hochgeladen wird
                // dropArea ohne Text für die Bildanzeige
                // bereit zum Senden
                dropArea.innerHTML = '';
                isValidImage = true;
                validityChanged();
              }
              else {
                // Es gibt mehrere Bilder --> der Nutzer darf Frame Delay einstellen
                // Frame Delay wird freigeschaltet mit Default maxDelay
                transitionTimeField.disabled = false;
                transitionTimeField.value = maxDelay;
              }
            }

            //#region Bildanzeige
            // einzelne Bildwrapper für das Bild, was angezeigt werden soll
            const imageWrapper = document.createElement('div');
            imageWrapper.className = 'image-wrapper';

            const dispImage = document.createElement('img');
            dispImage.src = URL.createObjectURL(files[i]);
            dispImage.style.maxWidth = '200px';
            dispImage.style.maxHeight = '100px';
            dispImage.style.marginBottom = '10px';
            dispImage.style.border = '1px solid #ccc';

            // einzelne kleine Buttons für Löschen eines Bilds
            const deleteButton = document.createElement('button');
            deleteButton.className = 'delete-button';
            deleteButton.innerText = 'x';
            deleteButton.name = uploadedImages.length - 1; // zugehörige Index damit das Bild auch vom Array entfernt werden kann

            deleteButton.addEventListener('click', (e) => {
              // beim Löschen wird das Bild vom Array und von der Anzeige entfernt
              preventDefaults(e);
              dropArea.removeChild(imageWrapper);
              uploadedImages.splice(parseInt(e.target.name), 1);
              if (uploadedImages.length == 1) {
                // es gibt nur noch 1 Bild
                // der Nutzer darf keinen Frame Delay einstellen
                transitionTimeField.disabled = true;
                transitionTimeField.value = "";
              }
              else if (uploadedImages.length == 0) {
                // es gibt kein Bild mehr, Senden nicht möglich
                // Text dropArea zurücksetzen
                isValidImage = false;
                validityChanged();
                dropArea.innerHTML = '<div class="container">\n<p class="centered-paragraph">Bilder hierher ziehen oder hier klicken...</p>\n</div>';
              }
            });

            // alles in dropArea anzeigen
            imageWrapper.appendChild(dispImage);
            imageWrapper.appendChild(deleteButton);
            dropArea.appendChild(imageWrapper);
            //#endregion
          }
        }
      }
    }

    async function processImages() {
      // Bilder und Delay in JSON Format vorbereiten
      const imagesWithDelay = {
        delay: transitionTimeField.value,
        images: []
      };

      for (let i = 0; i < uploadedImages.length; i++) {
        try {
          // jedes Bild in C Code Array umwandeln
          const cCodeSize = await processImg(uploadedImages[i]);
          imagesWithDelay.images.push(cCodeSize);
        }
        catch (error) {
          alert('Error: ' + error);
        }
      }

      return imagesWithDelay;
    }

    function processGif() {
      // GIF Frames und Delay in JSON Format vorbereiten
      const framesWithDelay = {
        delays: [],
        frames: []
      };

      // Gif laden und konvertiert jede Frames in C Code Array sowie stellt Delays zwischen den Frames fest
      return new Promise((resolve, reject) => {
        // konvertiert das Bild in ein C Code Array
        const reader = new FileReader();
        reader.onload = async function (e) {
          const gifData = new Uint8Array(e.target.result);
          const gifReader = new GifReader(gifData);

          for (let i = 0; i < gifReader.numFrames(); i++) {
            const frameInfo = gifReader.frameInfo(i);
            framesWithDelay.delays.push(frameInfo.delay * 10);

            const image = new ImageData(frameInfo.width, frameInfo.height);
            gifReader.decodeAndBlitFrameRGBA(i, image.data);

            let canvas = document.createElement('canvas');
            canvas.width = frameInfo.width;
            canvas.height = frameInfo.height;
            canvas.getContext('2d').putImageData(image, 0, 0);

            const cCodeSize = convertImgToCCodeSize(canvas);
            framesWithDelay.frames.push(cCodeSize);
          }

          resolve(framesWithDelay);
        };
        reader.readAsArrayBuffer(uploadedImages[0]);
        reader.onerror = (error) => reject(error);
      });
    }

    function processImg(file) {
      return new Promise((resolve, reject) => {
        // konvertiert das Bild in ein C Code Array
        const reader = new FileReader();
        reader.readAsDataURL(file);
        reader.onload = function () {
          let image = new Image();
          image.src = reader.result;
          image.onload = (e) => resolve(convertImgToCCodeSize(e.target));
        };
        reader.onerror = (error) => reject(error);
      });
    }

    function convertImgToCCodeSize(image) {
      let canvas = document.createElement('canvas');
      // Bild skalieren wenn nötig
      if (image.width > displayWidth || image.height > displayHeight) {
        let scale = Math.min(displayWidth / image.width, displayHeight / image.height);

        canvas.width = image.width * scale;
        canvas.height = image.height * scale;
      }
      else {
        canvas.width = image.width;
        canvas.height = image.height;
      }

      // Bild für die LED-Anzeige verarbeiten
      let context = canvas.getContext('2d');
      context.drawImage(image, 0, 0, canvas.width, canvas.height);

      let imageData = context.getImageData(0, 0, canvas.width, canvas.height);
      let pixelData = imageData.data;

      // pixel data berechnen für das C Code Array, es wird mit der Größe des Bilds in JSON Format vorbereitet 
      const cCodeSize = {
        size: [canvas.width, canvas.height],
        hexValues: []
      };

      for (var i = 0; i < pixelData.length; i += 4) {
        let r = pixelData[i];
        let g = pixelData[i + 1];
        let b = pixelData[i + 2];

        let uint16Value = ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
        let hexValue = uint16Value.toString(16).toUpperCase().padStart(4, '0');
        cCodeSize.hexValues.push('0x' + hexValue);
      }

      return cCodeSize;
    }

    function hexToRGB(hexValue) {
      // Umwandlung Farben von Hex Werten in RGB Werte
      return ['0x' + hexValue[1] + hexValue[2] | 0, '0x' + hexValue[3] + hexValue[4] | 0, '0x' + hexValue[5] + hexValue[6] | 0];
    }

    function validityChanged() {
      // Bild hochladen nur möglich, wenn die eingegebene Datei und Delay valid sind
      if (isValidImage && isValidDelay)
        imgSendButton.disabled = false;
      else
        imgSendButton.disabled = true;

      // Text hochladen nur möglich, wenn der Text, die Farbe und der Modus valid sind
      if (isValidText && isValidColor && isValidMode)
        txtSendButton.disabled = false;
      else
        txtSendButton.disabled = true;
    }

    function preventDefaults(event) {
      // Reset vermeiden
      event.preventDefault();
      event.stopPropagation();
    }

    function resetImages() {
      // Zurücksetzen der HTML Seite, dass kein Bild mehr angezeigt wird und der Button ausgegraut wird
      dropArea.innerHTML = '<div class="container">\n<p class="centered-paragraph">Bilder hierher ziehen oder hier klicken...</p>\n</div>';
      uploadedImages = [];
      transitionTimeField.disabled = true;
      transitionTimeField.value = "";
      isValidImage = false;
      validityChanged();
    }
    //#endregion
  </script>
</body>
)=====";