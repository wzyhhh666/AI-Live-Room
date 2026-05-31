function createProblemDetails({ type, title, status, detail, instance, errors }) {
  return {
    type: `https://api.chatroom.example/errors/${type}`,
    title,
    status,
    detail,
    instance,
    timestamp: new Date().toISOString(),
    ...(errors && { errors })
  }
}

class AppError extends Error {
  constructor(status, type, title, detail, errors) {
    super(detail)
    this.name = 'AppError'
    this.status = status
    this.type = type
    this.title = title
    this.detail = detail
    this.errors = errors
  }
}

function errorHandler(err, req, res, next) {
  if (err instanceof AppError) {
    return res.status(err.status).json(createProblemDetails({
      type: err.type,
      title: err.title,
      status: err.status,
      detail: err.detail,
      instance: req.originalUrl,
      errors: err.errors
    }))
  }

  if (err.status && err.message) {
    return res.status(err.status).json(createProblemDetails({
      type: 'request-error',
      title: err.message,
      status: err.status,
      detail: err.message,
      instance: req.originalUrl
    }))
  }

  console.error('Unhandled error:', err)
  res.status(500).json(createProblemDetails({
    type: 'internal-error',
    title: 'Internal Server Error',
    status: 500,
    detail: 'An unexpected error occurred',
    instance: req.originalUrl
  }))
}

function notFoundHandler(req, res) {
  res.status(404).json(createProblemDetails({
    type: 'not-found',
    title: 'Not Found',
    status: 404,
    detail: `Route ${req.method} ${req.originalUrl} not found`,
    instance: req.originalUrl
  }))
}

module.exports = { errorHandler, notFoundHandler, AppError, createProblemDetails }
