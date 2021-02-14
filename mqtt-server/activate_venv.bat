:start
IF EXIST ".venv" (
call .venv\Scripts\activate
) ELSE (
call virtualenv .venv
GOTO start
)

pip install -r requirements.txt