import csv
import serial
import serial.tools.list_ports
from datetime import datetime

PORT = "COM6"       # CAMBIA ESTO
BAUDRATE = 115200

HEADERS = [
    "fecha_pc",
    "hora_pc",
    "fecha_nodo",
    "hora_nodo",
    "bpm_avg",
    "bpm_inst",
    "finger",
    "raw_pulse",
    "mv_pulse",
    "ax_g",
    "ay_g",
    "az_g",
    "gx_dps",
    "gy_dps",
    "gz_dps",
    "imu_temp_c",
    "frame"
]

def listar_puertos():
    puertos = list(serial.tools.list_ports.comports())
    print("Puertos disponibles:")
    for p in puertos:
        print(f"  {p.device} - {p.description}")

def parse_line(line: str):
    line = line.strip()

    if not line.startswith("$NODO1,"):
        return None

    parts = line.split(",")

    # Esperamos:
    # $NODO1,fecha,hora,bpm_avg,bpm_inst,finger,raw,mv,ax,ay,az,gx,gy,gz,temp,frame
    if len(parts) != 16:
        return None

    now = datetime.now()
    fecha_pc = now.strftime("%Y-%m-%d")
    hora_pc = now.strftime("%H:%M:%S")

    return [
        fecha_pc,
        hora_pc,
        parts[1],   # fecha_nodo
        parts[2],   # hora_nodo
        parts[3],   # bpm_avg
        parts[4],   # bpm_inst
        parts[5],   # finger
        parts[6],   # raw_pulse
        parts[7],   # mv_pulse
        parts[8],   # ax_g
        parts[9],   # ay_g
        parts[10],  # az_g
        parts[11],  # gx_dps
        parts[12],  # gy_dps
        parts[13],  # gz_dps
        parts[14],  # imu_temp_c
        parts[15],  # frame
    ]

def main():
    listar_puertos()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"datos_nodo1_{timestamp}.csv"

    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    except Exception as e:
        print(f"\nNo se pudo abrir el puerto {PORT}: {e}")
        return

    with open(output_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(HEADERS)

        print(f"\nGuardando datos en: {output_file}")
        print("Presiona Ctrl+C para detener.\n")

        while True:
            try:
                raw = ser.readline().decode("utf-8", errors="ignore").strip()
                if not raw:
                    continue

                print(raw)

                parsed = parse_line(raw)
                if parsed:
                    writer.writerow(parsed)
                    f.flush()

            except KeyboardInterrupt:
                print("\nCaptura detenida.")
                break

if __name__ == "__main__":
    main()