#ifndef TEMPLATE_H
#define TEMPLATE_H

const char html_template[] = "<!DOCTYPE html>\n"
"<html lang=\"pt\">\n"
"<head>\n"
"  <meta charset=\"UTF-8\">\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"  <link href=\"https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap\" rel=\"stylesheet\">\n"
"  <title>House Control</title>\n"
"  <style>\n"
"    body {\n"
"      margin: 0;\n"
"      padding: 0;\n"
"      font-family: 'Roboto', sans-serif;\n"
"      background: #f0f2f5;\n"
"    }\n"
"    .container {\n"
"      max-width: 600px;\n"
"      margin: 50px auto;\n"
"      background: #ffffff;\n"
"      border-radius: 8px;\n"
"      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);\n"
"      padding: 20px;\n"
"    }\n"
"    h1,\n"
"    h2 {\n"
"      text-align: center;\n"
"      color: #333333;\n"
"    }\n"
"    .control-section {\n"
"      margin: 20px 0;\n"
"    }\n"
"    .control-section p {\n"
"      text-align: center;\n"
"      margin: 10px 0;\n"
"    }\n"
"    .button {\n"
"      display: inline-block;\n"
"      padding: 12px 24px;\n"
"      margin: 5px;\n"
"      border: none;\n"
"      border-radius: 4px;\n"
"      background: #6200EE;\n"
"      color: #ffffff;\n"
"      text-decoration: none;\n"
"      font-weight: 500;\n"
"      transition: background 0.3s ease;\n"
"    }\n"
"    .button:hover {\n"
"      background: #3700B3;\n"
"    }\n"
"    .status {\n"
"      font-size: 0.9em;\n"
"      color: #555555;\n"
"      text-align: center;\n"
"      margin-top: 20px;\n"
"    }\n"
"  </style>\n"
"</head>\n"
"<body>\n"
"  <div class=\"container\">\n"
"    <h1>House Control</h1>\n"
"    <div class=\"control-section\">\n"
"      <h2>ILUMINAÇÃO (Cômodos)</h2>\n"
"      <p><strong>SALA:</strong> <a class=\"button\" href=\"/led1/on\">Ligar</a> <a class=\"button\" href=\"/led1/off\">Desligar</a></p>\n"
"      <p><strong>COZINHA:</strong> <a class=\"button\" href=\"/led2/on\">Ligar</a> <a class=\"button\" href=\"/led2/off\">Desligar</a></p>\n"
"      <p><strong>QUARTO:</strong> <a class=\"button\" href=\"/led3/on\">Ligar</a> <a class=\"button\" href=\"/led3/off\">Desligar</a></p>\n"
"    </div>\n"
"    <div class=\"control-section\">\n"
"      <h2>ALARME</h2>\n"
"      <p><a class=\"button\" href=\"/buzzer/on\">Ativar</a> <a class=\"button\" href=\"/buzzer/off\">Desativar</a></p>\n"
"    </div>\n"
"    <div class=\"control-section\">\n"
"      <h2>STATUS</h2>\n"
"      <p>Sensor A: <span id=\"sensorStatus\">%s</span></p>\n"
"      <p>%s</p>\n"
"      <p><a class=\"button\" href=\"/status\">Update Status</a></p>\n"
"    </div>\n"
"  </div>\n"
"</body>\n"
"</html>";

#endif // TEMPLATE_H
