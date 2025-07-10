defmodule Hezi.Repo.Migrations.CreateGame do
  use Ecto.Migration

  def change do
    create table(:game) do
      add :room, :string

      timestamps(type: :utc_datetime)
    end
  end
end
