from app.services.spotify_service import spotify_service
from app.services.database_service import db_service

CLIENT_ID = "e5ec975b985241828b90e9316a6361af"
CLIENT_SECRET = "5ad664bd89b340028d035e5f3ef847de"

spotify_service.save_credentials(CLIENT_ID, CLIENT_SECRET)
print("SUCCESS: Spotify credentials persisted in SQLite database!")
