defmodule Hezi.Repo do
  use Ecto.Repo,
    otp_app: :hezi,
    adapter: Ecto.Adapters.Postgres
end
