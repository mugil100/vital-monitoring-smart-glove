export default function NotificationPanel({ criticalSoldiers }) {
  if (criticalSoldiers.length === 0) return null;

  return (
    <div className="notification-panel">
      {criticalSoldiers.map((name) => (
        <div key={name} className="critical-soldier">
          {name} is in critical condition!
        </div>
      ))}
    </div>
  );
}
