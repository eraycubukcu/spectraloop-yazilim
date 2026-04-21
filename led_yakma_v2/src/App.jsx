import { useState, useEffect, useRef } from "react";

function App() {
  const [telemetry, setTelemetry] = useState({
    durum: "offline",
    uptime: 0,
    gecikme: 999,
    sicaklik: 0,
  });
  const [hz, setHz] = useState(0);
  const [isLedOn, setIsLedOn] = useState(false);

  const PI_IP = "10.208.151.177"; // Pi IP adresin

  const packetCount = useRef(0);
  const lastUptimeRef = useRef(0);

  // 1. VERİ ÇEKME DÖNGÜSÜ (Gelişmiş Polling)
  useEffect(() => {
    let isMounted = true;
    let timeoutId;

    const fetchData = async () => {
      // İsteklerin üst üste binmesini engellemek için AbortController
      const controller = new AbortController();
      const fetchTimeout = setTimeout(() => controller.abort(), 400); // 400ms cevap gelmezse iptal et

      try {
        const res = await fetch(`http://${PI_IP}:5000/telemetry?t=${Date.now()}`, {
          signal: controller.signal
        });
        
        const data = await res.json();
        
        if (isMounted) {
          // Gelen veride durum yoksa bile online kabul et (cevap geldiğine göre)
          setTelemetry({ ...data, durum: data.durum || "online" });

          // Uptime kontrolü ile paket sayımı (Hz hesaplama için)
          if (data.uptime !== lastUptimeRef.current) {
            packetCount.current += 1;
            lastUptimeRef.current = data.uptime;
          }
        }
      } catch (e) {
        if (isMounted) {
          setTelemetry((p) => ({ ...p, durum: "offline" }));
          packetCount.current = 0; // Bağlantı koptuğunda Hz direkt sıfırlansın
        }
      } finally {
        clearTimeout(fetchTimeout);
        // Bir sonraki isteği sadece bu istek bittikten sonra planla (request piling koruması)
        if (isMounted) {
          timeoutId = setTimeout(fetchData, 100); 
        }
      }
    };

    fetchData();

    return () => {
      isMounted = false;
      clearTimeout(timeoutId);
    };
  }, []);

  // 2. HZ HESAPLAYICI (Saniyede bir packetCount'u sıfırlar)
  useEffect(() => {
    const timer = setInterval(() => {
      setHz(packetCount.current);
      packetCount.current = 0;
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const toggleLed = async () => {
    const action = isLedOn ? "off" : "on";
    try {
      const res = await fetch(`http://${PI_IP}:5000/led`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action }),
      });
      if (res.ok) setIsLedOn(!isLedOn);
    } catch (e) {
      console.error("LED Kontrol Hatası");
    }
  };

  const isConnected = telemetry.durum !== "offline";

  return (
    <div style={s.container}>
      <h2 style={s.title}>SPECTRALOOP TELEMETRİ v2</h2>

      <div style={s.grid}>
        {/* BAĞLANTI DURUMU */}
        <div style={{ ...s.card, borderColor: isConnected ? "#00ff00" : "#ff0000" }}>
          <div style={{ ...s.dot, backgroundColor: isConnected ? "#00ff00" : "#ff0000", boxShadow: isConnected ? "0 0 10px #00ff00" : "none" }} />
          <p style={s.label}>BAĞLANTI</p>
          <small style={{ color: isConnected ? "#00ff00" : "#ff0000" }}>{telemetry.durum.toUpperCase()}</small>
        </div>

        {/* VERİ HIZI (Hz) */}
        <div style={s.card}>
          <h1 style={{ color: "#00d4ff", margin: 0 }}>
            {isConnected ? hz : 0} <span style={{ fontSize: "14px" }}>Hz</span>
          </h1>
          <p style={s.label}>VERİ HIZI</p>
        </div>

        {/* SICAKLIK GÖSTERGESİ */}
        <div style={s.card}>
          <h1 style={{ color: "#ff9f43", margin: 0 }}>
            {isConnected ? Number(telemetry.sicaklik || 0).toFixed(1) : "--"}°C
          </h1>
          <p style={s.label}>SICAKLIK</p>
        </div>
      </div>

      {/* LED GÖRSELİ */}
      <div style={{ ...s.ledDisplay, backgroundColor: isLedOn ? "#00ff00" : "#222", boxShadow: isLedOn ? "0 0 40px #00ff00" : "none", borderColor: isLedOn ? "#00ff00" : "#333" }} />

      <button onClick={toggleLed} disabled={!isConnected} style={{ ...s.btn, backgroundColor: isLedOn ? "#ff4d4d" : "#00d4ff", opacity: isConnected ? 1 : 0.5 }}>
        {isLedOn ? "LED KAPAT (STOP)" : "LED AÇ (START)"}
      </button>

      <div style={s.footer}>
        Uptime: {telemetry.uptime} ms | Gecikme: {telemetry.gecikme.toFixed(2)}s
      </div>
    </div>
  );
}

const s = {
  container: { minHeight: "100vh", backgroundColor: "#050505", color: "white", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", fontFamily: "monospace", padding: "20px" },
  grid: { display: "flex", flexWrap: "wrap", gap: "20px", marginBottom: "40px", justifyContent: "center" },
  card: { background: "#111", border: "1px solid #222", padding: "25px", borderRadius: "15px", minWidth: "180px", textAlign: "center", transition: "0.3s" },
  label: { fontSize: "10px", color: "#666", marginTop: "10px", fontWeight: "bold", letterSpacing: "1px" },
  dot: { width: "10px", height: "10px", borderRadius: "50%", margin: "0 auto 10px", transition: "0.3s" },
  ledDisplay: { width: "60px", height: "60px", borderRadius: "50%", marginBottom: "30px", border: "4px solid #333", transition: "0.4s" },
  btn: { padding: "18px 50px", color: "#000", border: "none", borderRadius: "50px", fontWeight: "bold", cursor: "pointer", fontSize: "14px", letterSpacing: "1px", transition: "0.3s" },
  title: { letterSpacing: "8px", color: "#444", marginBottom: "60px", fontSize: "16px" },
  footer: { marginTop: "40px", color: "#333", fontSize: "10px", opacity: 0.5 },
};

export default App;