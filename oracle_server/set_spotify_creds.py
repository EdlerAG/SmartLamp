import os
import sys
from app.services.spotify_service import spotify_service
from app.services.database_service import db_service


def main() -> None:
    client_id = None
    client_secret = None

    if len(sys.argv) >= 3:
        client_id = sys.argv[1].strip()
        client_secret = sys.argv[2].strip()
    else:
        client_id = os.getenv("SPOTIFY_CLIENT_ID", "").strip()
        client_secret = os.getenv("SPOTIFY_CLIENT_SECRET", "").strip()

    if not client_id or not client_secret:
        print("Usage:")
        print("  python set_spotify_creds.py <CLIENT_ID> <CLIENT_SECRET>")
        print("Or set SPOTIFY_CLIENT_ID and SPOTIFY_CLIENT_SECRET environment variables.")
        sys.exit(1)

    spotify_service.save_credentials(client_id, client_secret)
    print("SUCCESS: Spotify credentials persisted in SQLite database!")


if __name__ == "__main__":
    main()
