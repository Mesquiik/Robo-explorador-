from flask import Flask, request, jsonify, render_template_string
import sqlite3
import os
from datetime import datetime

app = Flask(__name__)

DB_PATH = "robo_explorador.db"  

def init_db():
    """Cria o banco de dados e a tabela se não existirem."""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS leituras (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp        TEXT    NOT NULL,
            temperatura_c    REAL    NOT NULL,
            umidade_pct      REAL    NOT NULL,
            luminosidade     INTEGER NOT NULL,
            presenca         INTEGER NOT NULL,
            probabilidade_vida REAL  NOT NULL
        )
    """)
    conn.commit()
    conn.close()
    print(f"[DB] Banco de dados pronto: {DB_PATH}")

def get_db():
    """Retorna uma conexão com o banco."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row   
    return conn


@app.route("/leituras", methods=["POST"])
def receber_leitura():
    dados = request.get_json()

    campos = ["temperatura_c", "umidade_pct", "luminosidade", "presenca", "probabilidade_vida"]
    for campo in campos:
        if campo not in dados:
            return jsonify({"erro": f"Campo ausente: {campo}"}), 400

    timestamp = dados.get("timestamp", datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"))

    conn = get_db()
    conn.execute("""
        INSERT INTO leituras (timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida)
        VALUES (?, ?, ?, ?, ?, ?)
    """, (
        timestamp,
        dados["temperatura_c"],
        dados["umidade_pct"],
        dados["luminosidade"],
        dados["presenca"],
        dados["probabilidade_vida"]
    ))
    conn.commit()
    conn.close()

    print(f"[DB] Leitura salva: temp={dados['temperatura_c']}°C  prob={dados['probabilidade_vida']}%")
    return jsonify({"status": "ok"}), 201

@app.route("/leituras", methods=["GET"])
def listar_leituras():
    conn = get_db()
    rows = conn.execute("""
        SELECT * FROM leituras
        ORDER BY id DESC
        LIMIT 100
    """).fetchall()
    conn.close()

    leituras = [dict(row) for row in rows]
    return jsonify(leituras)

PAGINA_HTML = """
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Robô Explorador - Dashboard</title>
  <style>
    body { font-family: monospace; background: #0d1117; color: #c9d1d9; margin: 20px; }
    h1   { color: #58a6ff; }
    table { border-collapse: collapse; width: 100%; margin-top: 16px; }
    th, td { border: 1px solid #30363d; padding: 8px 12px; text-align: center; }
    th   { background: #161b22; color: #58a6ff; }
    tr:nth-child(even) { background: #161b22; }
    .alerta { color: #f85149; font-weight: bold; }
    .normal { color: #3fb950; }
    button  { background: #238636; color: white; border: none; padding: 8px 16px;
              cursor: pointer; font-size: 14px; border-radius: 4px; margin-bottom: 12px; }
    button:hover { background: #2ea043; }
  </style>
</head>
<body>
  <h1>🤖 Robô Explorador — Últimas Leituras</h1>
  <button onclick="carregar()">🔄 Atualizar</button>
  <table id="tabela">
    <thead>
      <tr>
        <th>#</th>
        <th>Timestamp</th>
        <th>Temp (°C)</th>
        <th>Umidade (%)</th>
        <th>Luminosidade</th>
        <th>Presença</th>
        <th>Prob. Vida (%)</th>
        <th>Status</th>
      </tr>
    </thead>
    <tbody id="corpo"></tbody>
  </table>

  <script>
    async function carregar() {
      const resp = await fetch('/leituras');
      const dados = await resp.json();
      const corpo = document.getElementById('corpo');
      corpo.innerHTML = '';
      dados.forEach(d => {
        const alerta = d.probabilidade_vida > 75;
        corpo.innerHTML += `
          <tr>
            <td>${d.id}</td>
            <td>${d.timestamp}</td>
            <td>${d.temperatura_c}</td>
            <td>${d.umidade_pct}</td>
            <td>${d.luminosidade}</td>
            <td>${d.presenca ? 'Sim' : 'Não'}</td>
            <td class="${alerta ? 'alerta' : 'normal'}">${d.probabilidade_vida}</td>
            <td class="${alerta ? 'alerta' : 'normal'}">${alerta ? '🚨 ALERTA' : '✅ Normal'}</td>
          </tr>`;
      });
    }
    carregar();
    setInterval(carregar, 5000); // atualiza a cada 5 segundos
  </script>
</body>
</html>
"""

@app.route("/")
def dashboard():
    return render_template_string(PAGINA_HTML)


if __name__ == "__main__":
    init_db()
    print("[SERVER] Rodando em http://0.0.0.0:5000")
    print("[SERVER] Dashboard: abra http://localhost:5000 no navegador")
    app.run(host="0.0.0.0", port=5000, debug=True)
