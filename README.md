# X-HEEP-based FPGA EMUlation Platform (FEMU)

This repository is based on [x-heep-femu-sdk](https://github.com/esl-epfl/x-heep-femu-sdk/tree/main). Please refer to that repository for full documentation, board setup, SDK usage, and general structure.

---

## 🛠️ Custom Modifications in This Version

This fork includes hardware and software modifications focused on enabling Trusted Execution Environment (TEE) experimentation using the X-HEEP platform.

### ✅ Hardware Configuration
- **Core:** [cv32e20](https://github.com/openhwgroup/cv32e20) RISC-V core
- **Memory Protection:** **2 PMP (Physical Memory Protection) regions**
- **Memory System:** **16 memory banks** in the data RAM subsystem

---

### ✅ Software Extensions (Under `sw/riscv/apps/`)

#### 🔹 `pmp_test/`
- A minimal C application to test PMP region configuration and access behavior.
- Useful for debugging PMP faults and verifying secure/unsecure memory separation.

#### 🔹 `tflite_scpi/`
- A secure (TEE) version of the TensorFlow Lite inference runtime.
- Combines **user-mode input handling** and **machine-mode inference** via ECALL dispatching.
- Demonstrates privilege separation between SCPI command parsing and ML workload.

#### 🔹 `tflite_scpi_m/`
- A monolithic **machine-mode only** version of the same TensorFlow Lite inference.
- Cloned and adapted from: [x-heep-femu-tflite-sdk](https://github.com/cad-polito-it/x-heep-femu-tflite-sdk)

---

📌 These additions enable exploring software/hardware co-design techniques for low-overhead TEE runtime isolation on resource-constrained MCUs.

---

*Last updated: 08 July 2025*
