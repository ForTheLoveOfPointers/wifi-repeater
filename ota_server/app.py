import os
from flask import Flask, send_from_directory, request, jsonify

UPLOAD_API_KEY = "EXAMPLE_KEY" #Ideally, you would use .env variables for this. It is merely an example

app = Flask(__name__)

#
# VERSION SHOULD CHANGE VIA WORKFLOWS TO BE THE SAME AS THE TAG, BUT OF COURSE
# THAT WOULD NEED A DIFFERENT REPO OR TAG, SO IT IS NOT IMPLEMENTED HERE.
#
# THE IMPLEMENTATION WOULD BE SENDING THE VERSION AS METADATA ALONG WITH THE NEW FIRMWARE FILE,
# AND THEN THE /upload_firmware ROUTE WOULD UPDATE THE LATEST_VERSION.
#
# FOR THIS, THE VERSION SHOULD BE SAVED IN A JSON FILE, FOR EXAMPLE.
#
LATEST_VERSION = "0.0.1"
FIRMWARE_DIR = "firmware"
FIRMWARE_NAME = "firmware.bin"
FIRMWARE_NAME_NO_EXT = FIRMWARE_NAME.split(".")[0]

@app.route("/upload_firmware", methods=["POST"])
def upload_firmware():
    # Validate API key
    key = request.headers.get("X-API-KEY")
    if key != UPLOAD_API_KEY:
        return jsonify({"error": "unauthorized"}), 401

    if FIRMWARE_NAME_NO_EXT not in request.files:
        return jsonify({"error": "no file uploaded"}), 400

    file = request.files[FIRMWARE_NAME_NO_EXT]

    save_path = os.path.join(FIRMWARE_DIR, FIRMWARE_NAME)
    file.save(save_path)

    return jsonify({"status": "ok", "saved": save_path})

@app.route("/update-check")
def update_check():
    return jsonify({"version": LATEST_VERSION})

@app.route("/upgrade")
def upgrade():
    return send_from_directory(FIRMWARE_DIR, FIRMWARE_NAME, as_attachment=True)

if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        ssl_context=("cert.pem", "key.pem")
    )