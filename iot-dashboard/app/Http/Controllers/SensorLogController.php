<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use App\Models\SensorLog;

class SensorLogController extends Controller
{
    public function store(Request $request){
        $request->validate([
            'rms_x' => 'required|numeric',
            'rms_z' => 'required|numeric',
            'arus' => 'required|numeric',
        ]);

        $batas_getaran = 2.5;
        $batas_arus = 5.0;

        $isAnomaliGetaran = ($request->rms_x > $batas_getaran || $request->rms_z > $batas_getaran);
        $isAnomaliArus = ($request->arus > $batas_arus);

        $sensorData = SensorLog::create([
            'rms_x' => $request->rms_x,
            'rms_z' => $request->rms_z,
            'arus' => $request->arus,
            'anomali_getaran' => $isAnomaliGetaran,
            'anomali_arus' => $isAnomaliArus,
        ]);

        return response()->json([
            'status' => 'success',
            'message' => 'Data sensor berhasil disimpan',
            'data' => $sensorData
        ], 201);
    }

    public function getLatest()
    {
        $latestData = SensorLog::latest()->first();
        return response()->json($latestData);
    }
}
