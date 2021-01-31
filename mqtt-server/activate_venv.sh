if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi

source .venv/scripts/activate

pip install requirements.txt