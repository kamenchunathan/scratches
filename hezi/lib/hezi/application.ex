defmodule Hezi.Application do
  # See https://hexdocs.pm/elixir/Application.html
  # for more information on OTP Applications
  @moduledoc false

  use Application

  @impl true
  def start(_type, _args) do
    children = [
      HeziWeb.Telemetry,
      Hezi.Repo,
      {DNSCluster, query: Application.get_env(:hezi, :dns_cluster_query) || :ignore},
      {Phoenix.PubSub, name: Hezi.PubSub},
      # Start a worker by calling: Hezi.Worker.start_link(arg)
      # {Hezi.Worker, arg},
      # Start to serve requests, typically the last entry
      HeziWeb.Endpoint
    ]

    # See https://hexdocs.pm/elixir/Supervisor.html
    # for other strategies and supported options
    opts = [strategy: :one_for_one, name: Hezi.Supervisor]
    Supervisor.start_link(children, opts)
  end

  # Tell Phoenix to update the endpoint configuration
  # whenever the application is updated.
  @impl true
  def config_change(changed, _new, removed) do
    HeziWeb.Endpoint.config_change(changed, removed)
    :ok
  end
end
