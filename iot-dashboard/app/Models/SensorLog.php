<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Factories\HasFactory;

class SensorLog extends Model
{
    use HasFactory;

    protected $fillable = [
        'rms_x',
        'rms_z',
        'arus',
        'anomali_getaran',
        'anomali_arus',
    ];
}
