import { useEffect, useState } from "react";
import { Line } from "react-chartjs-2";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from "chart.js";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const MQTT_BROKER_URL = "wss://broker.hivemq.com:8884/mqtt";
const LED_COMMAND_TOPIC = "led/control";
const GAS_VALUE_TOPIC = "gas/value";
const LED_STATUS_TOPIC = "led/status";
const TEMPERATURE_TOPIC = "dht/temperature";
const HUMIDITY_TOPIC = "dht/humidity";

export default function MqttDashboard() {
  const [gasValue, setGasValue] = useState("-");
  const [temperature, setTemperature] = useState("-");
  const [humidity, setHumidity] = useState("-");
  const [ledStatus, setLedStatus] = useState("Desconhecido");
  const [client, setClient] = useState<any | null>(null);

  const [gasData, setGasData] = useState<number[]>([]);
  const [temperatureData, setTemperatureData] = useState<number[]>([]);
  const [humidityData, setHumidityData] = useState<number[]>([]);
  const [timestamps, setTimestamps] = useState<string[]>([]);

  useEffect(() => {
    const mqttClient = (window as any).mqtt.connect(MQTT_BROKER_URL);

    mqttClient.on("connect", () => {
      console.log("Conectado ao broker MQTT");
      mqttClient.subscribe(GAS_VALUE_TOPIC);
      mqttClient.subscribe(LED_STATUS_TOPIC);
      mqttClient.subscribe(TEMPERATURE_TOPIC);
      mqttClient.subscribe(HUMIDITY_TOPIC);
    });

    mqttClient.on("message", (topic: string, message: Buffer) => {
      const value = parseFloat(message.toString());
      const timestamp = new Date().toLocaleTimeString();

      if (topic === GAS_VALUE_TOPIC) {
        setGasValue(value.toString());
        setGasData((prev) => [...prev, value].slice(-20)); 
      } else if (topic === TEMPERATURE_TOPIC) {
        setTemperature(value.toString());
        setTemperatureData((prev) => [...prev, value].slice(-20));
      } else if (topic === HUMIDITY_TOPIC) {
        setHumidity(value.toString());
        setHumidityData((prev) => [...prev, value].slice(-20));
      } else if (topic === LED_STATUS_TOPIC) {
        setLedStatus(message.toString());
      }

      setTimestamps((prev) => [...prev, timestamp].slice(-20)); 
    });

    setClient(mqttClient);

    return () => {
      mqttClient.end();
    };
  }, []);

  const handleLedToggle = (turnOn: boolean) => {
    if (client) {
      const message = turnOn ? "L" : "D";
      client.publish(LED_COMMAND_TOPIC, message);
    }
  };

  const gasChartData = {
    labels: timestamps,
    datasets: [
      {
        label: "Valor do Gás",
        data: gasData,
        borderColor: "rgb(75, 192, 192)",
        backgroundColor: "rgba(75, 192, 192, 0.2)",
      },
    ],
  };

  const temperatureChartData = {
    labels: timestamps,
    datasets: [
      {
        label: "Temperatura (°C)",
        data: temperatureData,
        borderColor: "rgb(255, 99, 132)",
        backgroundColor: "rgba(255, 99, 132, 0.2)",
      },
    ],
  };

  const humidityChartData = {
    labels: timestamps,
    datasets: [
      {
        label: "Umidade (%)",
        data: humidityData,
        borderColor: "rgb(54, 162, 235)",
        backgroundColor: "rgba(54, 162, 235, 0.2)",
      },
    ],
  };

  const chartOptions = {
    responsive: true,
    plugins: {
      legend: {
        position: "top" as const,
      },
      title: {
        display: true,
        text: "Gráficos Temporais dos Sensores",
      },
    },
  };

  return (
    <div className="p-6 max-w-md mx-auto bg-white rounded-xl shadow-md space-y-4">
      <h1 className="text-2xl font-bold">Painel de Controle MQTT</h1>

      <div>
        <p className="text-gray-700">Valor do Gás:</p>
        <p className="text-lg font-mono">{gasValue}</p>
      </div>

      <div>
        <p className="text-gray-700">Temperatura:</p>
        <p className="text-lg font-mono">{temperature} °C</p>
      </div>

      <div>
        <p className="text-gray-700">Umidade:</p>
        <p className="text-lg font-mono">{humidity} %</p>
      </div>

      <div>
        <p className="text-gray-700">Status do LED:</p>
        <p className="text-lg font-semibold">{ledStatus}</p>
      </div>

      <div className="flex gap-4">
        <button
          className="px-4 py-2 bg-blue-500 text-white rounded hover:bg-blue-600"
          onClick={() => handleLedToggle(true)}
        >
          Ligar LED
        </button>
        <button
          className="px-4 py-2 bg-gray-200 text-gray-700 rounded hover:bg-gray-300"
          onClick={() => handleLedToggle(false)}
        >
          Desligar LED
        </button>
      </div>

      <div className="mt-6">
        <h2 className="text-xl font-bold">Gráfico do Gás</h2>
        <Line data={gasChartData} options={chartOptions} />
      </div>

      <div className="mt-6">
        <h2 className="text-xl font-bold">Gráfico de Temperatura</h2>
        <Line data={temperatureChartData} options={chartOptions} />
      </div>

      <div className="mt-6">
        <h2 className="text-xl font-bold">Gráfico de Umidade</h2>
        <Line data={humidityChartData} options={chartOptions} />
      </div>
    </div>
  );
}