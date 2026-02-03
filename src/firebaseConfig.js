import { initializeApp } from "firebase/app";
import { getAuth } from "firebase/auth";
import { getFirestore } from "firebase/firestore";
import { getAnalytics } from "firebase/analytics";

const firebaseConfig = {
  apiKey: "AIzaSyATCTIoGNFg6aT2zm5xu9XfeqiNgoUOVMQ",
  authDomain: "smart-glove-d9194.firebaseapp.com",
  projectId: "smart-glove-d9194",
  storageBucket: "smart-glove-d9194.firebasestorage.app",
  messagingSenderId: "641012584459",
  appId: "1:641012584459:web:b0fba1ff3c75302025770e",
  measurementId: "G-78GCKT581N"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const auth = getAuth(app);
export const db = getFirestore(app);
export const analytics = getAnalytics(app);