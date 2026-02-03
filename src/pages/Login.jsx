import { useState } from "react";
import { signInWithEmailAndPassword } from "firebase/auth";
import { auth } from "../firebaseConfig";
import { useNavigate } from "react-router-dom";

export default function Login() {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const navigate = useNavigate();

  const handleLogin = async () => {
    try {
      await signInWithEmailAndPassword(auth, email, password);
      
      navigate("/dashboard");
    } catch (err) {
      alert(err.message);
    }
  };

  return (
    <div className="login-container">
  <div className="login-box">
    <h2>Basecamp Login</h2>
    <input placeholder="Email" onChange={(e) => setEmail(e.target.value)} />
    <input placeholder="Password" type="password" onChange={(e) => setPassword(e.target.value)} />
    <button onClick={handleLogin}>Login</button>
  </div>
</div>

  );
}
