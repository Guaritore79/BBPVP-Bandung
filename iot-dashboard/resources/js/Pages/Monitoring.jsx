import React, { useState, useEffect } from 'react';
import { Head } from '@inertiajs/react';

export default function Monitoring() {
    // State untuk menyimpan data sensor terbaru
    const [sensorData, setSensorData] = useState({
        rms_x: 0, rms_z: 0, arus: 0, anomali_getaran: false, created_at: '-'
    });
    
    // State untuk menampung riwayat data (untuk didownload AI)
    const [dataLog, setDataLog] = useState([]);

    // Fungsi otomatis berjalan saat halaman dibuka
    useEffect(() => {
        const interval = setInterval(() => {
            fetch('/api/sensor/latest')
                .then(res => res.json())
                .then(data => {
                    if (data) {
                        setSensorData(data);
                        // Simpan ke riwayat dataset
                        setDataLog(prev => [...prev, data]);
                    }
                })
                .catch(err => console.error("Gagal ambil data:", err));
        }, 1000); // Tarik data tiap 1 detik

        // Bersihkan interval jika halaman ditutup
        return () => clearInterval(interval);
    }, []);

    // Fungsi Download CSV
    const handleDownloadCSV = () => {
        if (dataLog.length === 0) {
            alert("Belum ada data terekam!");
            return;
        }

        let csvContent = "waktu,rms_x,rms_z,arus_ampere,anomali\n";
        dataLog.forEach(row => {
            csvContent += `${row.created_at},${row.rms_x},${row.rms_z},${row.arus},${row.anomali_getaran ? 1 : 0}\n`;
        });

        const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement("a");
        link.href = URL.createObjectURL(blob);
        link.download = `dataset_getaran_${Date.now()}.csv`;
        link.click();
    };

    return (
        <div className="min-h-screen bg-gray-900 text-white p-8 font-sans">
            <Head title="Monitoring Getaran" />
            
            <div className="max-w-3xl mx-auto space-y-6">
                <div className="text-center">
                    <h1 className="text-4xl font-bold text-blue-400">Dashboard Prediktif</h1>
                    <p className="text-gray-400 mt-2">Sistem Pemantauan Getaran Motor & Arus</p>
                </div>

                {/* Indikator Status & Anomali */}
                <div className={`p-4 rounded-lg text-center font-bold text-xl ${sensorData.anomali_getaran ? 'bg-red-600' : 'bg-green-600'}`}>
                    {sensorData.anomali_getaran ? "⚠️ PERINGATAN: GETARAN TINGGI (ANOMALI)!" : "✅ MESIN BERJALAN NORMAL"}
                </div>

                {/* Kartu Sensor */}
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                    <div className="bg-gray-800 p-6 rounded-lg text-center shadow-lg border border-gray-700">
                        <h3 className="text-gray-400 text-sm">Getaran (X-Axis)</h3>
                        <p className="text-3xl font-mono mt-2">{Number(sensorData.rms_x).toFixed(3)}</p>
                    </div>
                    <div className="bg-gray-800 p-6 rounded-lg text-center shadow-lg border border-gray-700">
                        <h3 className="text-gray-400 text-sm">Getaran (Z-Axis)</h3>
                        <p className="text-3xl font-mono mt-2">{Number(sensorData.rms_z).toFixed(3)}</p>
                    </div>
                    <div className="bg-gray-800 p-6 rounded-lg text-center shadow-lg border border-gray-700">
                        <h3 className="text-gray-400 text-sm">Arus Motor</h3>
                        <p className="text-3xl font-mono mt-2 text-yellow-400">{Number(sensorData.arus).toFixed(2)} A</p>
                    </div>
                </div>

                {/* Tombol Download CSV untuk Dataset AI */}
                <div className="mt-8 bg-gray-800 p-6 rounded-lg text-center border border-gray-700">
                    <button 
                        onClick={handleDownloadCSV}
                        className="bg-blue-600 hover:bg-blue-500 text-white px-6 py-3 rounded-lg font-bold transition-all"
                    >
                        ⬇️ Download Dataset CSV ({dataLog.length} baris)
                    </button>
                    <p className="text-gray-500 text-xs mt-2">*Biarkan halaman ini terbuka untuk merekam lebih banyak data pelatihan AI.</p>
                </div>
            </div>
        </div>
    );
}