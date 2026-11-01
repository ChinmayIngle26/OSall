"""
Simple AI/ML application: Iris flower classifier served over HTTP (Flask + scikit-learn).

Endpoints
  GET  /          -> short help
  GET  /health    -> liveness check
  POST /predict   -> JSON {"features": [sepal_len, sepal_wid, petal_len, petal_wid]}
  GET  /load?n=N  -> run N predictions (use it to generate CPU load for monitoring)
"""
import os
import time

import numpy as np
from flask import Flask, jsonify, request
from sklearn.datasets import load_iris
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split

app = Flask(__name__)

# ---- train the model once at start-up ----
_iris = load_iris()
_X_train, _X_test, _y_train, _y_test = train_test_split(
    _iris.data, _iris.target, test_size=0.2, random_state=42)
_model = RandomForestClassifier(n_estimators=100, random_state=42).fit(_X_train, _y_train)
_ACCURACY = float(_model.score(_X_test, _y_test))


@app.get("/")
def index():
    return jsonify(message="Iris classifier running inside a container",
                   accuracy=round(_ACCURACY, 3),
                   usage='POST /predict with {"features": [5.1, 3.5, 1.4, 0.2]}')


@app.get("/health")
def health():
    return jsonify(status="ok", pid=os.getpid())


@app.post("/predict")
def predict():
    data = request.get_json(silent=True) or {}
    feats = data.get("features")
    if not isinstance(feats, list) or len(feats) != 4:
        return jsonify(error="send JSON like {'features': [5.1, 3.5, 1.4, 0.2]}"), 400
    try:
        x = np.array(feats, dtype=float).reshape(1, -1)
    except (TypeError, ValueError):
        return jsonify(error="features must be numbers"), 400
    cls = int(_model.predict(x)[0])
    proba = _model.predict_proba(x)[0].tolist()
    return jsonify(prediction=_iris.target_names[cls], class_index=cls,
                   probabilities=[round(p, 3) for p in proba])


@app.get("/load")
def load():
    n = min(max(int(request.args.get("n", 200)), 1), 5000)
    t0 = time.time()
    for _ in range(n):
        _model.predict(_X_test)
    return jsonify(predictions=n * len(_X_test), seconds=round(time.time() - t0, 3))


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 5000)))
