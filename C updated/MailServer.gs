// ========== CONFIGURATION ========== //
const FIREBASE_URL = "https://smart-garage-door-6bed5-default-rtdb.firebaseio.com/pins/driver_pin.json";
const AUTH_TOKEN = "YnO9U41SPHDTJ3lYD46UABVGFZdqlKtPqqWsM80a";
const RECIPIENT_EMAIL = "samithahemantha00415@gmail.com";

// ========== MAIN FUNCTION ========== //
function checkPinChangeAndAlert() {
  try {
    const authedUrl = `${FIREBASE_URL}?auth=${AUTH_TOKEN}`;
    const options = {
      muteHttpExceptions: true,
      headers: { 'Accept': 'application/json' }
    };

    const response = UrlFetchApp.fetch(authedUrl, options);
    const responseBody = response.getContentText();
    const responseCode = response.getResponseCode();

    if (responseCode === 200) {
      const currentPin = JSON.parse(responseBody);

      if (!currentPin) {
        throw new Error("PIN not found in Firebase response. Response was: " + responseBody);
      }

      const scriptProperties = PropertiesService.getScriptProperties();
      const lastPin = scriptProperties.getProperty("lastPin");

      if (currentPin !== lastPin) {
        sendPinEmail(currentPin);
        scriptProperties.setProperty("lastPin", currentPin);
        console.log(`PIN changed. Email sent: ${currentPin}`);
      } else {
        console.log(`PIN unchanged (${currentPin}). No email sent.`);
      }
    } else {
      sendErrorEmail(`Firebase Error ${responseCode}`, responseBody);
    }
  } catch (e) {
    sendErrorEmail("Script Error", e.toString());
  }
}

// ========== EMAIL FUNCTIONS ========== //
function sendPinEmail(newPin) {
  const subject = `New Garage PIN`;
  const body = `
    <h2>New Garage Access PIN</h2>
    <p>Authorized vehicle detected at the garage. Please autheticate with the pin<p>
    <p><strong>PIN:</strong> ${newPin}</p>
    <p><strong>Time:</strong> ${new Date()}</p>
    <p><a href="${FIREBASE_URL}">Check PIN in Firebase</a></p>
  `;

  GmailApp.sendEmail(RECIPIENT_EMAIL, subject, "", {
    htmlBody: body
  });
}

function sendErrorEmail(errorTitle, errorDetails) {
  GmailApp.sendEmail(RECIPIENT_EMAIL, `❌ Garage Script Error: ${errorTitle}`, `
    Error Details:
    ${errorDetails}
    
    Firebase URL: ${FIREBASE_URL}
    Time: ${new Date()}
  `);
}

// ========== TEST FUNCTION ========== //
function testPinCheck() {
  checkPinChangeAndAlert();
}
