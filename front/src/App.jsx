import { useState, useEffect, useRef } from "react";

function App() {
  const [telemetry, setTelemetry] = useState({
    S1: 0,
    S2: 0,
    S3: 0,
    HZ: 0, // STM32'nin hesapladığı iç döngü hızı
    UP: 0, // STM32 Uptime (Saniye)
    durum: "offline",
  });

  const [pollingHz, setPollingHz] = useState(0); // Web'in veri çekme hızı
  const [isLedOn, setIsLedOn] = useState(false);

  const PI_IP = "10.208.151.177"; // Pi IP adresin

  const packetCount = useRef(0);
  const lastUpRef = useRef(0);

  // 1. VERİ ÇEKME DÖNGÜSÜ
  useEffect(() => {
    let isMounted = true;
    let timeoutId;

    const fetchData = async () => {
      const controller = new AbortController();
      const fetchTimeout = setTimeout(() => controller.abort(), 400);

      try {
        // Cache engellemek için timestamp ekliyoruz
        const res = await fetch(
          `http://${PI_IP}:5000/telemetry?t=${Date.now()}`,
          {
            signal: controller.signal,
          },
        );

        const data = await res.json();

        if (isMounted) {
          setTelemetry({ ...data, durum: "online" });

          // Paket sayımı (Polling Hz hesaplama için)
          // Eğer UP (uptime) değişmişse yeni paket gelmiş demektir
          if (data.UP !== lastUpRef.current) {
            packetCount.current += 1;
            lastUpRef.current = data.UP;
          }
        }
      } catch (e) {
        if (isMounted) {
          setTelemetry((p) => ({ ...p, durum: "offline" }));
        }
      } finally {
        clearTimeout(fetchTimeout);
        if (isMounted) {
          timeoutId = setTimeout(fetchData, 50); // Hız için 50ms polling
        }
      }
    };

    fetchData();
    return () => {
      isMounted = false;
      clearTimeout(timeoutId);
    };
  }, []);

  // 2. POLLING HZ HESAPLAYICI (Web sayfasının saniyede kaç kez veri aldığı)
  useEffect(() => {
    const timer = setInterval(() => {
      setPollingHz(packetCount.current);
      packetCount.current = 0;
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const toggleLed = async () => {
    const action = isLedOn ? "off" : "on";
    try {
      await fetch(`http://${PI_IP}:5000/led`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action }),
      });
      setIsLedOn(!isLedOn);
    } catch (e) {
      console.error("LED Hatası");
    }
  };

  const isConnected = telemetry.durum !== "offline";

  return (
    <div style={s.container}>
      <header style={s.header}>
        <h2 style={s.title}>SPECTRALOOP MULTI-SENSOR TELEMETRY</h2>
        <div style={s.statusBadge}>
          <div
            style={{
              ...s.dot,
              backgroundColor: isConnected ? "#00ff00" : "#ff0000",
            }}
          />
          <span>{isConnected ? "SYSTEM ACTIVE" : "CONNECTION LOST"}</span>
        </div>
      </header>

      {/* ANA VERİ GRİDİ */}
      <div style={s.mainGrid}>
        {/* SICAKLIK KARTLARI */}
        <div style={s.sensorGroup}>
          {[
            {
              id: "S1",
              val: telemetry.S1,
              color: "#ff4d4d",
              label: "BOILER TEMP",
            },
            {
              id: "S2",
              val: telemetry.S2,
              color: "#ff9f43",
              label: "ENGINE TEMP",
            },
            {
              id: "S3",
              val: telemetry.S3,
              color: "#00d4ff",
              label: "EXHAUST TEMP",
            },
          ].map((sensor) => (
            <div key={sensor.id} style={s.card}>
              <p style={s.label}>{sensor.label}</p>
              <h1 style={{ ...s.value, color: sensor.color }}>
                {isConnected ? sensor.val.toFixed(2) : "--.--"}°
              </h1>
              <small style={s.subLabel}>{sensor.id} CHANNEL</small>
            </div>
          ))}
        </div>

        {/* SİSTEM PERFORMANS KARTLARI */}
        <div style={s.performanceGroup}>
          <div style={s.cardSmall}>
            <p style={s.label}>STM32 INTERNAL HZ</p>
            <h2 style={{ color: "#00ff00" }}>
              {isConnected ? telemetry.HZ.toFixed(1) : 0} <small>Hz</small>
            </h2>
          </div>
          <div style={s.cardSmall}>
            <p style={s.label}>WEB POLLING HZ</p>
            <h2 style={{ color: "#00d4ff" }}>
              {pollingHz} <small>Hz</small>
            </h2>
          </div>
          <div style={s.cardSmall}>
            <p style={s.label}>SYSTEM UPTIME</p>
            <h2 style={{ color: "#fff" }}>
              {isConnected ? telemetry.UP : 0} <small>s</small>
            </h2>
          </div>
        </div>
      </div>

      {/* KONTROL PANELİ */}
      <div style={s.controlPanel}>
        <div
          style={{
            ...s.ledInd,
            backgroundColor: isLedOn ? "#00ff00" : "#1a1a1a",
            boxShadow: isLedOn ? "0 0 30px #00ff00" : "none",
          }}
        />
        <button
          onClick={toggleLed}
          disabled={!isConnected}
          style={{
            ...s.btn,
            background: isLedOn
              ? "linear-gradient(145deg, #ff4d4d, #b30000)"
              : "linear-gradient(145deg, #00d4ff, #0082a3)",
          }}
        >
          {isLedOn ? "DEACTIVATE SYSTEM" : "ACTIVATE SYSTEM"}
        </button>
      </div>

      <footer style={s.footer}>
        DEVICE_IP: {PI_IP} | TELEMETRY_STREAM_V3.0
      </footer>
    </div>
  );
}

const s = {
  container: {
    minHeight: "100vh",
    backgroundColor: "#080808",
    color: "#eee",
    display: "flex",
    flexDirection: "column",
    alignItems: "center",
    fontFamily: "'Inter', sans-serif",
    padding: "40px",
  },
  header: { textAlign: "center", marginBottom: "50px" },
  title: {
    letterSpacing: "5px",
    color: "#555",
    fontSize: "14px",
    fontWeight: "900",
  },
  statusBadge: {
    display: "flex",
    alignItems: "center",
    gap: "10px",
    justifyContent: "center",
    fontSize: "10px",
    marginTop: "10px",
    color: "#444",
  },
  dot: { width: "8px", height: "8px", borderRadius: "50%" },

  mainGrid: { width: "100%", maxWidth: "900px" },
  sensorGroup: {
    display: "grid",
    gridTemplateColumns: "repeat(auto-fit, minmax(250px, 1fr))",
    gap: "20px",
    marginBottom: "20px",
  },
  performanceGroup: {
    display: "grid",
    gridTemplateColumns: "repeat(3, 1fr)",
    gap: "20px",
  },

  card: {
    background: "linear-gradient(145deg, #111, #0a0a0a)",
    border: "1px solid #1a1a1a",
    padding: "30px",
    borderRadius: "20px",
    textAlign: "center",
  },
  cardSmall: {
    background: "#0a0a0a",
    border: "1px solid #1a1a1a",
    padding: "15px",
    borderRadius: "15px",
    textAlign: "center",
  },

  value: { fontSize: "48px", margin: "10px 0", fontWeight: "300" },
  label: {
    fontSize: "10px",
    color: "#555",
    fontWeight: "bold",
    letterSpacing: "2px",
  },
  subLabel: { fontSize: "9px", color: "#333" },

  controlPanel: {
    marginTop: "50px",
    display: "flex",
    flexDirection: "column",
    alignItems: "center",
    gap: "20px",
  },
  ledInd: {
    width: "15px",
    height: "15px",
    borderRadius: "50%",
    border: "2px solid #333",
  },
  btn: {
    padding: "15px 40px",
    border: "none",
    borderRadius: "10px",
    color: "white",
    fontWeight: "bold",
    cursor: "pointer",
    transition: "0.3s",
  },
  footer: {
    marginTop: "auto",
    color: "#222",
    fontSize: "10px",
    letterSpacing: "1px",
  },
};

export default App;
