import hashlib
from pathlib import Path

# --- Configuration ---
BASE_DIR = Path(__file__).resolve().parent     # folder where script resides
DEVICE_ID_FILE = BASE_DIR / "DEVICE_ID"
DEVICE_KEY_FILE = BASE_DIR / "DEVICE_KEY"
SALT = "Q9m#T4vP!s2Lx8Z@"  # same salt as C++ code

_cached_device_key = None


def compute_sha1(text: str) -> str:
    """Return SHA1 hex digest of a string."""
    return hashlib.sha1(text.encode("utf-8")).hexdigest()


def get_device_id() -> str:
    """Read device ID from file (cross-platform path) and ensure it's not empty."""
    if not DEVICE_ID_FILE.exists():
        raise FileNotFoundError(f"Device ID file not found: {DEVICE_ID_FILE}")

    content = DEVICE_ID_FILE.read_text(encoding="utf-8").strip()

    if not content:
        print(f"ERROR: Device ID file is empty: {DEVICE_ID_FILE}")
        print(f"Please populate the file with a valid device ID before running this script.")
        exit(1)

    return content



def save_device_key(key: str):
    """Write the generated key to a file (cross-platform path)."""
    DEVICE_KEY_FILE.write_text(key, encoding="utf-8")


def get_device_key() -> str:
    global _cached_device_key

    # return cached result if available
    if _cached_device_key:
        return _cached_device_key

    # Step 1: SHA1(deviceID)
    device_id = get_device_id()
    first_hash = compute_sha1(device_id)

    # Step 2: SHA1(first_hash + salt)
    first_hash = compute_sha1(first_hash + SALT)

    # Step 3: SHA1(second_hash)
    second_hash = compute_sha1(first_hash)

    # Step 4: first_hash + last 2 chars of second_hash
    _cached_device_key = first_hash + second_hash[-2:]

    # Save to file
    save_device_key(_cached_device_key)

    return _cached_device_key


if __name__ == "__main__":
    device_id = get_device_id()
    print("Device ID: ", device_id)
    device_key = get_device_key()
    print("Device Key:", device_key)
    print("Key saved to:", DEVICE_KEY_FILE)
