import { useState, useEffect, useRef } from 'react';

function App() {
  const [telemetry, setTelemetry] = useState({ durum: 'offline', uptime: 0, gecikme: 999 });
  const [hz, setHz] = useState(0);
  const [isLedOn, setIsLedOn] = useState(false);
  
  const PI_IP = "10.208.151.177";
  
  const packetCount = useRef(0);
  const lastUptimeRef = useRef(0);

  // 1. VERİ ÇEKME DÖNGÜSÜ (Fetch)
  useEffect(() => {
    let isMounted = true;
    const fetchData = async () => {
      try {
        const res = await fetch(`http://${PI_IP}:5000/telemetry?t=${Date.now()}`);
        const data = await res.json();
        
        if (isMounted) {
          setTelemetry(data);
          // Eğer STM32'den gelen uptime değişmişse, yeni bir paket gelmiş demektir
          if (data.uptime !== lastUptimeRef.current) {
            packetCount.current += 1;
            lastUptimeRef.current = data.uptime;
          }
        }
      } catch (e) {
        if (isMounted) setTelemetry(p => ({ ...p, durum: 'offline' }));
      }
      if (isMounted) setTimeout(fetchData, 80); // ~12Hz ile sorgula ki 10Hz'i kaçırmayalım
    };
    fetchData();
    return () => isMounted = false;
  }, []);

  // 2. HZ HESAPLAMA (Saniyede 1 kez sıfırlanır)
  useEffect(() => {
    const timer = setInterval(() => {
      setHz(packetCount.current);
      packetCount.current = 0;
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const toggleLed = async () => {
    const action = isLedOn ? 'off' : 'on';
    try {
      await fetch(`http://${PI_IP}:5000/led`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action }),
      });
      setIsLedOn(!isLedOn);
    } catch (e) { console.error("LED Kontrol Hatası"); }
  };

  const isConnected = telemetry.durum !== 'offline';

  return (
    <div style={s.container}>
      <h2 style={s.title}>TELEMETRİ VERİLERİ</h2>
      
      <div style={s.grid}>
        <div style={s.card}>
          <div style={{...s.dot, backgroundColor: isConnected ? '#00ff00' : '#ff0000'}} />
          <p>BAĞLANTI</p>
          <small>{telemetry.durum.toUpperCase()}</small>
        </div>

        <div style={s.card}>
          <h1 style={{color: '#00d4ff', margin: 0}}>{isConnected ? hz : 0} Hz</h1>
          <p>VERİ HIZI</p>
        </div>
      </div>

      <div style={{...s.ledDisplay, backgroundColor: isLedOn ? '#00ff00' : '#222', boxShadow: isLedOn ? '0 0 20px #00ff00' : 'none'}} />

      <button onClick={toggleLed} disabled={!isConnected} style={{...s.btn, opacity: isConnected ? 1 : 0.5}}>
        {isLedOn ? 'LED KAPAT' : 'LED AÇ'}
      </button>

      <div style={s.footer}>
        Uptime: {telemetry.uptime} ms | Gecikme: {telemetry.gecikme.toFixed(2)}s
      </div>
    </div>
  );
}

const s = {
  container: { height: '100vh', backgroundColor: '#0a0a0a', color: 'white', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', fontFamily: 'monospace' },
  grid: { display: 'flex', gap: '20px', marginBottom: '30px' },
  card: { background: '#111', border: '1px solid #333', padding: '20px', borderRadius: '12px', width: '180px', textAlign: 'center' },
  dot: { width: '12px', height: '12px', borderRadius: '50%', margin: '0 auto 10px' },
  ledDisplay: { width: '50px', height: '50px', borderRadius: '50%', marginBottom: '20px', transition: '0.3s' },
  btn: { padding: '15px 40px', backgroundColor: '#00d4ff', border: 'none', borderRadius: '8px', fontWeight: 'bold', cursor: 'pointer', fontSize: '16px' },
  title: { letterSpacing: '5px', color: '#555', marginBottom: '40px' },
  footer: { marginTop: '30px', color: '#ffffff', fontSize: '12px' }
};

export default App;