# Drawn Out: When "Theoretical Draws" Aren't Actually Drawn

Investigate the "el-classico" - reference from a GMHikaru titled tuesday video where while in between rounds keeps finding people playing out rook endgames. Chess players seem to hope the oponnent blunders or maybe flags. So how likely is this?

We're specifically looking to calculate:

1. The percentage of theoretically drawn rook endgames that are needlessly played out (because apparently shaking hands is too difficult)
2. How many excruciating moves players torture each other with before someone finally loses a drawn position
3. Whether there's a correlation between tournament round and moves played out

## Architecture

This project scrapes data from Titled tuesday tournament, and analyzes rook endgames to study patterns in draws and outcomes. The system consists of two main components:

1. **Data Collection Service**: Scrapes tournament data and game PGNs from Chess.com
2. **Game Analysis Service**: Analyzes collected games to identify and study rook endgames

### Stack

- **Database**: Turso (SQLite-compatible distributed database)
- **Web Scraping**: Selenium for browser automation
- **Data Processing**: Python with chess.pgn library
- **Deployment**: Sevalla using docker containers

## Prerequisites

- Docker
- Access to ghcr.io for container images
- Turso database credentials

## Environment Variables

The following environment variables are required:

```
DB_URI=libsql://your-database-url
DB_TOKEN=your-turso-auth-token
SELENIUM_HOST=http://selenium
SELENIUM_PORT=4444
```

## Deployment

The project is deployed using two Docker containers:

1. **Application Container**: `ghcr.io/your-org/drawn-out:latest`
2. **Selenium Container**: Standard Selenium Chrome container with increased shared memory

### Docker Compose Example

```yaml
version: "3"

services:
  app:
    image: ghcr.io/your-org/drawn-out:latest
    environment:
      - DB_URI=${DB_URI}
      - DB_TOKEN=${DB_TOKEN}
      - SELENIUM_HOST=http://selenium
      - SELENIUM_PORT=4444
    depends_on:
      - selenium
    networks:
      - chess-net

  selenium:
    image: selenium/standalone-chrome:latest
    shm_size: 2g
    networks:
      - chess-net

networks:
  chess-net:
```

## Database Schema

The database contains the following tables:

### Game Table

- `id`: Chess.com game ID (primary key)
- `status`: Processing status (not_asked, success, error)
- `retries`: Number of retry attempts for failed games
- `pgn`: PGN notation of the chess game
- `tournament_id`: Reference to parent tournament
- `round`: Round number in the tournament

### Tournament Table

- `id`: Tournament ID (primary key)
- `rounds_fetched`: Number of rounds processed from this tournament

### Jobs Table

- `id`: Job ID (primary key)
- `status`: Job status (not_asked, pending, complete)
- `tournament_id`: Tournament being processed

## How It Works

### Data Collection Process

1. The system starts by processing jobs from the database.
2. For each pending tournament:
   - Fetches game IDs for each round of the tournament
   - For each game ID, uses Selenium to navigate to the game page and extract PGN data
   - Implements robust retry logic with exponential backoff
   - Tracks performance metrics for monitoring

### Game Analysis

The analysis pipeline:

1. Loads completed games from the database
2. Analyzes the final position to identify rook endgames
3. Determines if games were played out or ended in draws
4. Records the reason for draws and winners where applicable
5. Exports the analyzed data to CSV

## Rook Endgame Analysis

The system specifically looks for endgames with only kings and rooks remaining on the board. It tracks:

- Whether the position is a true rook endgame
- How many moves were played in the endgame
- The outcome (draw or win)
- Reasons for draws (stalemate, insufficient material, etc.)

## Metrics and Monitoring

The system tracks performance metrics including:

- Selenium request times
- Database call intervals
- Selenium connection times

## Running Locally

To run the project locally:

1. Clone the repository
2. Set up required environment variables
3. Run with Docker Compose:
   ```
   docker-compose up
   ```

## Adding New Tournament Jobs

To add a new tournament for processing:

```sql
INSERT INTO tournament (id, rounds_fetched) VALUES ('tournament-id', 0);
INSERT INTO jobs (tournament_id, status) VALUES ('tournament-id', 'not_asked');
```

## Development

Uses nix for a development shell `run nix develop` to get a shell or just use the docker container

## Troubleshooting

- If Selenium is failing frequently, try increasing the shared memory size.
- For database connection issues, verify your Turso credentials and network access.
- The data collection has built-in retry logic for transient failures.
